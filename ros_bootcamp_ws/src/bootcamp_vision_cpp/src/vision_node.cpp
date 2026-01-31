#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <std_msgs/msg/string.hpp>
#include <cv_bridge/cv_bridge.h>
#include <ament_index_cpp/get_package_share_directory.hpp>

#include "bootcamp_vision_cpp/inference_backend.hpp"
#include "bootcamp_vision_cpp/srv/enroll_face.hpp"
#include "bootcamp_vision_cpp/srv/list_faces.hpp"
#include "bootcamp_vision_cpp/srv/delete_face.hpp"

using namespace bootcamp_vision_cpp;

class VisionNode : public rclcpp::Node
{
public:
  VisionNode()
  : Node("vision_node")
  {
    this->declare_parameter("yolo_model", "tinyyolov2.onnx");
    this->declare_parameter("yolo_conf", 0.35);
    this->declare_parameter("face_rec_model", "w600k_r50.onnx");
    this->declare_parameter("face_thresh", 0.45);

    yolo_conf_ = this->get_parameter("yolo_conf").as_double();
    face_thresh_ = this->get_parameter("face_thresh").as_double();

    // Always use OpenCV DNN backend
    backend_ = create_backend("opencv");
    if (!backend_) {
      RCLCPP_ERROR(this->get_logger(), "Failed to create OpenCV DNN backend");
      throw std::runtime_error("Backend creation failed");
    }

    RCLCPP_INFO(this->get_logger(), "Using backend: %s (CUDA: %s)", 
                backend_->name().c_str(), 
                backend_->supports_cuda() ? "yes" : "no");

    this->declare_parameter("enable_face_detection", false);
    enable_faces_ = this->get_parameter("enable_face_detection").as_bool();

    std::string yolo_model = this->get_parameter("yolo_model").as_string();
    if (!backend_->load_yolo_model(yolo_model)) {
      RCLCPP_ERROR(this->get_logger(), "Failed to load YOLO model: %s", yolo_model.c_str());
      throw std::runtime_error("YOLO model loading failed");
    }

    if (enable_faces_) {
      // Get package share directory for model paths
      std::string pkg_share = ament_index_cpp::get_package_share_directory("bootcamp_vision_cpp");
      std::string face_det_model = pkg_share + "/models/face_detector.caffemodel";
      std::string face_rec_model = this->get_parameter("face_rec_model").as_string();
      
      if (!backend_->load_face_models(face_det_model, face_rec_model)) {
        RCLCPP_WARN(this->get_logger(), "Failed to load face models");
      }
    } else {
      RCLCPP_INFO(this->get_logger(), "Face detection disabled (enable_face_detection=false)");
    }

    subscription_ = this->create_subscription<sensor_msgs::msg::Image>(
      "/camera/image_raw", 10,
      std::bind(&VisionNode::image_callback, this, std::placeholders::_1));

    pub_img_ = this->create_publisher<sensor_msgs::msg::Image>("/vision/annotated", 10);
    pub_faces_ = this->create_publisher<sensor_msgs::msg::Image>("/vision/faces", 10);
    pub_det_ = this->create_publisher<std_msgs::msg::String>("/vision/detections", 10);

    // Create face enrollment services
    srv_enroll_ = this->create_service<srv::EnrollFace>(
      "/vision/enroll_face",
      std::bind(&VisionNode::enroll_face_callback, this, 
                std::placeholders::_1, std::placeholders::_2));
    
    srv_list_ = this->create_service<srv::ListFaces>(
      "/vision/list_faces",
      std::bind(&VisionNode::list_faces_callback, this,
                std::placeholders::_1, std::placeholders::_2));
    
    srv_delete_ = this->create_service<srv::DeleteFace>(
      "/vision/delete_face",
      std::bind(&VisionNode::delete_face_callback, this,
                std::placeholders::_1, std::placeholders::_2));

    // Set up face database path
    const char* home = std::getenv("HOME");
    if (home) {
      db_path_ = std::string(home) + "/.ros/bootcamp_vision_cpp/faces_database.json";
      std::filesystem::create_directories(std::string(home) + "/.ros/bootcamp_vision_cpp");
      load_face_database();
    }

    RCLCPP_INFO(this->get_logger(), "Subscribed to /camera/image_raw");
    RCLCPP_INFO(this->get_logger(), "Publishing /vision/annotated (combined), /vision/faces (faces only), /vision/detections (JSON)");
    RCLCPP_INFO(this->get_logger(), "Services: /vision/enroll_face, /vision/list_faces, /vision/delete_face");
  }

private:
  void load_face_database()
  {
    if (!std::filesystem::exists(db_path_)) {
      RCLCPP_INFO(this->get_logger(), "No face database found at %s", db_path_.c_str());
      return;
    }
    
    std::ifstream file(db_path_);
    if (!file.is_open()) {
      RCLCPP_WARN(this->get_logger(), "Failed to open database: %s", db_path_.c_str());
      return;
    }
    
    // Simple JSON parsing (format: {"name": [emb1, emb2, ...], ...})
    std::string line, content;
    while (std::getline(file, line)) {
      content += line;
    }
    
    if (content.empty() || content == "{}" || content == "{\n}") {
      RCLCPP_INFO(this->get_logger(), "Face database is empty");
      return;
    }
    
    // Parse simple JSON format
    size_t pos = 0;
    int loaded = 0;
    while ((pos = content.find("\"", pos)) != std::string::npos) {
      size_t name_start = pos + 1;
      size_t name_end = content.find("\"", name_start);
      if (name_end == std::string::npos) break;
      
      std::string name = content.substr(name_start, name_end - name_start);
      
      size_t arr_start = content.find("[", name_end);
      size_t arr_end = content.find("]", arr_start);
      if (arr_start == std::string::npos || arr_end == std::string::npos) break;
      
      std::string emb_str = content.substr(arr_start + 1, arr_end - arr_start - 1);
      std::vector<float> embedding;
      
      std::stringstream ss(emb_str);
      std::string val;
      while (std::getline(ss, val, ',')) {
        try {
          embedding.push_back(std::stof(val));
        } catch (...) {}
      }
      
      if (embedding.size() == 512) {
        backend_->add_known_face(name, embedding);
        loaded++;
      }
      
      pos = arr_end + 1;
    }
    
    RCLCPP_INFO(this->get_logger(), "Loaded %d faces from database", loaded);
  }
  
  void save_face_database(const std::map<std::string, std::vector<float>>& faces)
  {
    std::ofstream file(db_path_);
    if (!file.is_open()) {
      RCLCPP_ERROR(this->get_logger(), "Failed to save database: %s", db_path_.c_str());
      return;
    }
    
    file << "{" << std::endl;
    bool first = true;
    for (const auto& [name, embedding] : faces) {
      if (!first) file << "," << std::endl;
      first = false;
      
      file << "  \"" << name << "\": [";
      for (size_t i = 0; i < embedding.size(); i++) {
        file << embedding[i];
        if (i < embedding.size() - 1) file << ",";
      }
      file << "]";
    }
    file << std::endl << "}" << std::endl;
    
    RCLCPP_INFO(this->get_logger(), "Saved %zu faces to database", faces.size());
  }

  void enroll_face_callback(
    const std::shared_ptr<srv::EnrollFace::Request> request,
    std::shared_ptr<srv::EnrollFace::Response> response)
  {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    
    if (latest_frame_.empty()) {
      response->success = false;
      response->message = "No camera frame available";
      return;
    }
    
    if (request->name.empty()) {
      response->success = false;
      response->message = "Name cannot be empty";
      return;
    }
    
    // Detect faces in current frame
    std::vector<FaceDetection> faces;
    try {
      faces = backend_->detect_faces(latest_frame_, 0.5);
    } catch (const std::exception& e) {
      response->success = false;
      response->message = std::string("Face detection failed: ") + e.what();
      return;
    }
    
    if (faces.empty()) {
      response->success = false;
      response->message = "No faces detected in current frame";
      return;
    }
    
    // Select largest face by bbox area
    size_t largest_idx = 0;
    int largest_area = 0;
    for (size_t i = 0; i < faces.size(); i++) {
      int area = faces[i].bbox.width * faces[i].bbox.height;
      if (area > largest_area) {
        largest_area = area;
        largest_idx = i;
      }
    }
    
    const auto& face = faces[largest_idx];
    
    // Extract embedding for the largest face
    cv::Mat face_roi = latest_frame_(face.bbox);
    std::vector<float> embedding = backend_->extract_face_embedding(face_roi);
    
    if (embedding.size() != 512) {
      response->success = false;
      response->message = "Failed to extract face embedding";
      return;
    }
    
    // Add to database
    backend_->add_known_face(request->name, embedding);
    
    // Save to disk
    auto all_faces = backend_->get_known_faces();
    save_face_database(all_faces);
    
    response->success = true;
    if (faces.size() > 1) {
      response->message = std::string("Enrolled ") + request->name + 
                         " (selected largest face from " + 
                         std::to_string(faces.size()) + " detected)";
    } else {
      response->message = std::string("Successfully enrolled ") + request->name;
    }
    response->embedding_id = request->name;
    
    RCLCPP_INFO(this->get_logger(), "Enrolled face: %s", request->name.c_str());
  }
  
  void list_faces_callback(
    const std::shared_ptr<srv::ListFaces::Request>,
    std::shared_ptr<srv::ListFaces::Response> response)
  {
    auto faces = backend_->get_known_faces();
    for (const auto& [name, _] : faces) {
      response->names.push_back(name);
    }
    response->count = faces.size();
  }
  
  void delete_face_callback(
    const std::shared_ptr<srv::DeleteFace::Request> request,
    std::shared_ptr<srv::DeleteFace::Response> response)
  {
    if (backend_->remove_known_face(request->name)) {
      auto all_faces = backend_->get_known_faces();
      save_face_database(all_faces);
      
      response->success = true;
      response->message = std::string("Deleted ") + request->name;
      RCLCPP_INFO(this->get_logger(), "Deleted face: %s", request->name.c_str());
    } else {
      response->success = false;
      response->message = std::string("Face not found: ") + request->name;
    }
  }


  std::string create_json_output(const std_msgs::msg::Header& header,
                                   const std::vector<FaceDetection>& faces,
                                   const std::vector<Detection>& objects)
  {
    std::stringstream ss;
    ss << "{";
    ss << "\"stamp\":{\"sec\":" << header.stamp.sec << ",\"nanosec\":" << header.stamp.nanosec << "},";
    
    ss << "\"faces\":[";
    for (size_t i = 0; i < faces.size(); i++) {
      const auto& f = faces[i];
      ss << "{\"bbox\":[" << f.bbox.x << "," << f.bbox.y << "," 
         << (f.bbox.x + f.bbox.width) << "," << (f.bbox.y + f.bbox.height) << "],";
      ss << "\"name\":\"" << f.name << "\",\"score\":" << f.score << "}";
      if (i < faces.size() - 1) ss << ",";
    }
    ss << "],";

    ss << "\"objects\":[";
    for (size_t i = 0; i < objects.size(); i++) {
      const auto& obj = objects[i];
      ss << "{\"bbox\":[" << obj.bbox.x << "," << obj.bbox.y << "," 
         << (obj.bbox.x + obj.bbox.width) << "," << (obj.bbox.y + obj.bbox.height) << "],";
      ss << "\"label\":\"" << obj.label << "\",\"conf\":" << obj.confidence << "}";
      if (i < objects.size() - 1) ss << ",";
    }
    ss << "]";
    
    ss << "}";
    return ss.str();
  }

  void image_callback(const sensor_msgs::msg::Image::SharedPtr msg)
  {
    cv_bridge::CvImagePtr cv_ptr;
    try {
      cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
    } catch (cv_bridge::Exception& e) {
      RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
      return;
    }

    cv::Mat frame = cv_ptr->image;
    
    // Cache latest frame for enrollment
    {
      std::lock_guard<std::mutex> lock(frame_mutex_);
      latest_frame_ = frame.clone();
    }

    std::vector<FaceDetection> faces;
    if (enable_faces_) {
      try {
        faces = backend_->detect_faces(frame, 0.5);
        RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 5000, 
                            "Detected %zu faces", faces.size());
      } catch (const std::exception& e) {
        RCLCPP_ERROR(this->get_logger(), "Face detection crashed: %s", e.what());
      }
    }
    
    // Create face-only annotated image
    cv::Mat face_frame = frame.clone();
    for (const auto& f : faces) {
      cv::Scalar color = (f.name != "unknown") ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
      cv::rectangle(face_frame, f.bbox, color, 2);
      std::stringstream label;
      label << f.name << " " << std::fixed << std::setprecision(2) << f.score;
      cv::putText(face_frame, label.str(), cv::Point(f.bbox.x, std::max(20, f.bbox.y - 10)),
                  cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2, cv::LINE_AA);
    }
    
    // Draw faces on combined frame too
    for (const auto& f : faces) {
      cv::Scalar color = (f.name != "unknown") ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
      cv::rectangle(frame, f.bbox, color, 2);
      std::stringstream label;
      label << f.name << " " << std::fixed << std::setprecision(2) << f.score;
      cv::putText(frame, label.str(), cv::Point(f.bbox.x, std::max(20, f.bbox.y - 10)),
                  cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2, cv::LINE_AA);
    }

    auto objects = backend_->detect_yolo(frame, yolo_conf_, 0.4);
    for (const auto& obj : objects) {
      cv::Scalar color = backend_->get_color_for_class(obj.class_id);
      cv::rectangle(frame, obj.bbox, color, 2);
      std::stringstream text;
      text << obj.label << " " << std::fixed << std::setprecision(2) << obj.confidence;
      cv::putText(frame, text.str(), cv::Point(obj.bbox.x, std::max(20, obj.bbox.y - 10)),
                  cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2, cv::LINE_AA);
    }

    auto det_msg = std_msgs::msg::String();
    det_msg.data = create_json_output(msg->header, faces, objects);
    pub_det_->publish(det_msg);

    std_msgs::msg::Header header;
    header.stamp = msg->header.stamp;
    header.frame_id = msg->header.frame_id;
    
    // Publish combined image (faces + objects)
    auto out_msg = cv_bridge::CvImage(header, "bgr8", frame).toImageMsg();
    pub_img_->publish(*out_msg);
    
    // Publish face-only image
    auto face_msg = cv_bridge::CvImage(header, "bgr8", face_frame).toImageMsg();
    pub_faces_->publish(*face_msg);
  }

  std::unique_ptr<InferenceBackend> backend_;
  double yolo_conf_;
  double face_thresh_;
  bool enable_faces_;

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr subscription_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_img_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_faces_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_det_;
  
  // Face enrollment services
  rclcpp::Service<srv::EnrollFace>::SharedPtr srv_enroll_;
  rclcpp::Service<srv::ListFaces>::SharedPtr srv_list_;
  rclcpp::Service<srv::DeleteFace>::SharedPtr srv_delete_;
  
  // Face database
  std::string db_path_;
  cv::Mat latest_frame_;
  std::mutex frame_mutex_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<VisionNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
