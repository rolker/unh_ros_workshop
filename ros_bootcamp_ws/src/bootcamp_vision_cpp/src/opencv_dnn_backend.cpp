#include "bootcamp_vision_cpp/opencv_dnn_backend.hpp"
#include <rclcpp/rclcpp.hpp>
#include <fstream>

namespace bootcamp_vision_cpp
{

// COCO class names for YOLO models
static const std::vector<std::string> COCO_CLASSES = {
  "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat",
  "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat",
  "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack",
  "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball",
  "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket",
  "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
  "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair",
  "couch", "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse",
  "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink",
  "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier",
  "toothbrush"
};

// VOC class names for TinyYOLOv2 (20 classes)
static const std::vector<std::string> VOC_CLASSES = {
  "aeroplane", "bicycle", "bird", "boat", "bottle",
  "bus", "car", "cat", "chair", "cow",
  "diningtable", "dog", "horse", "motorbike", "person",
  "pottedplant", "sheep", "sofa", "train", "tvmonitor"
};

OpenCVDNNBackend::OpenCVDNNBackend()
#ifdef USE_OPENCV_DNN
  : yolo_loaded_(false),
    face_models_loaded_(false),
    is_yolov5_(false),
    yolo_input_size_(640)
#endif
{
  RCLCPP_INFO(rclcpp::get_logger("OpenCVDNNBackend"), "OpenCV DNN backend created");
}

bool OpenCVDNNBackend::is_available() const
{
#ifdef USE_OPENCV_DNN
  return true;
#else
  return false;
#endif
}

bool OpenCVDNNBackend::load_yolo_model(const std::string& model_path)
{
#ifdef USE_OPENCV_DNN
  try {
    RCLCPP_INFO(rclcpp::get_logger("OpenCVDNNBackend"), 
                "Loading YOLO model: %s", model_path.c_str());
    
    // Determine YOLO version based on filename
    if (model_path.find("tinyyolov2") != std::string::npos) {
      is_yolov5_ = false;  // Using this flag for model type detection
      yolo_input_size_ = 416;  // TinyYOLOv2 uses 416x416
    } else if (model_path.find("yolov5") != std::string::npos) {
      is_yolov5_ = true;
      yolo_input_size_ = 640;
    } else {
      // Default assume newer YOLO
      is_yolov5_ = false;
      yolo_input_size_ = 640;
    }
    
    // Load the ONNX model
    yolo_net_ = cv::dnn::readNetFromONNX(model_path);
    
    if (yolo_net_.empty()) {
      RCLCPP_ERROR(rclcpp::get_logger("OpenCVDNNBackend"), 
                   "Failed to load ONNX model from: %s", model_path.c_str());
      yolo_loaded_ = false;
      return false;
    }
    
    // Set backend and target
    yolo_net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    yolo_net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    
    yolo_loaded_ = true;
    
    if (model_path.find("tinyyolov2") != std::string::npos) {
      RCLCPP_INFO(rclcpp::get_logger("OpenCVDNNBackend"), 
                  "Successfully loaded TinyYOLOv2 model with OpenCV DNN (416x416 input)");
    } else if (is_yolov5_) {
      RCLCPP_INFO(rclcpp::get_logger("OpenCVDNNBackend"), 
                  "Successfully loaded YOLOv5 model with OpenCV DNN");
    } else {
      RCLCPP_INFO(rclcpp::get_logger("OpenCVDNNBackend"), 
                  "Successfully loaded YOLO model with OpenCV DNN");
    }
    
    return true;
    
  } catch (const cv::Exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("OpenCVDNNBackend"), 
                 "OpenCV exception loading model: %s", e.what());
    yolo_loaded_ = false;
    return false;
  } catch (const std::exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("OpenCVDNNBackend"), 
                 "Exception loading model: %s", e.what());
    yolo_loaded_ = false;
    return false;
  }
#else
  RCLCPP_ERROR(
    rclcpp::get_logger("OpenCVDNNBackend"),
    "OpenCV DNN backend not available - not compiled with USE_OPENCV_DNN flag");
  return false;
#endif
}

std::vector<Detection> OpenCVDNNBackend::detect_yolo(
  const cv::Mat& image,
  float conf_threshold,
  float nms_threshold)
{
#ifdef USE_OPENCV_DNN
  std::vector<Detection> detections;
  
  if (!yolo_loaded_) {
    RCLCPP_WARN_THROTTLE(
      rclcpp::get_logger("OpenCVDNNBackend"),
      *rclcpp::Clock::make_shared(),
      5000,  // Log every 5 seconds
      "YOLO model not loaded");
    return detections;
  }
  
  try {
    // Prepare input blob
    cv::Mat blob;
    // TinyYOLOv2 expects unnormalized input [0, 255], not [0, 1]
    double scale_factor = (yolo_input_size_ == 416) ? 1.0 : 1.0/255.0;  // 416 = TinyYOLOv2
    cv::dnn::blobFromImage(image, blob, scale_factor, 
                          cv::Size(yolo_input_size_, yolo_input_size_), 
                          cv::Scalar(0, 0, 0), true, false);
    
    yolo_net_.setInput(blob);
    
    // Forward pass
    std::vector<cv::Mat> outputs;
    yolo_net_.forward(outputs, yolo_net_.getUnconnectedOutLayersNames());
    
    // Process outputs based on YOLO version
    // Check for TinyYOLOv2 output format first
    if (!outputs.empty() && outputs[0].dims == 4 && outputs[0].size[1] == 125) {
      // TinyYOLOv2 output format: [1, 125, 13, 13]
      // 125 = 5 * (5 + 20) where 5 is num anchors, 20 is num classes
      process_tinyyolov2_output(outputs, image, conf_threshold, nms_threshold, detections);
    } else if (is_yolov5_) {
      // YOLOv5 output format: [1, 25200, 85] for 640x640 input
      // Each detection: [x, y, w, h, objectness, class0_prob, class1_prob, ...]
      process_yolov5_output(outputs, image, conf_threshold, nms_threshold, detections);
    } else {
      // YOLOv8 output format - more complex, may not work well with OpenCV DNN
      RCLCPP_WARN_THROTTLE(
        rclcpp::get_logger("OpenCVDNNBackend"),
        *rclcpp::Clock::make_shared(),
        10000,  // Log every 10 seconds
        "YOLOv8 with OpenCV DNN may have compatibility issues. Consider using YOLOv5.");
      process_yolov8_output(outputs, image, conf_threshold, nms_threshold, detections);
    }
    
  } catch (const cv::Exception& e) {
    RCLCPP_ERROR_THROTTLE(
      rclcpp::get_logger("OpenCVDNNBackend"),
      *rclcpp::Clock::make_shared(),
      5000,
      "OpenCV exception during inference: %s", e.what());
  } catch (const std::exception& e) {
    RCLCPP_ERROR_THROTTLE(
      rclcpp::get_logger("OpenCVDNNBackend"),
      *rclcpp::Clock::make_shared(),
      5000,
      "Exception during inference: %s", e.what());
  }
  
  return detections;
#else
  RCLCPP_ERROR(
    rclcpp::get_logger("OpenCVDNNBackend"),
    "OpenCV DNN backend not available");
  return std::vector<Detection>();
#endif
}

bool OpenCVDNNBackend::load_face_models(
  const std::string& det_model,
  const std::string& rec_model)
{
#ifdef USE_OPENCV_DNN
  try {
    // Load face detector (Caffe model - res10_300x300_ssd)
    RCLCPP_INFO(rclcpp::get_logger("OpenCVDNNBackend"), 
                "Loading face detector: %s", det_model.c_str());
    
    // For ResNet-SSD face detector, we need both .prototxt and .caffemodel
    // Assuming det_model is the .caffemodel path
    std::string prototxt_path = det_model;
    // Replace .caffemodel with .prototxt or look for it
    size_t pos = prototxt_path.find(".caffemodel");
    if (pos != std::string::npos) {
      prototxt_path.replace(pos, 11, ".prototxt");
    }
    
    face_det_net_ = cv::dnn::readNetFromCaffe(prototxt_path, det_model);
    RCLCPP_INFO(rclcpp::get_logger("OpenCVDNNBackend"), 
                "Face detector loaded successfully");
    
    // Load face recognition model (ArcFace ONNX)
    RCLCPP_INFO(rclcpp::get_logger("OpenCVDNNBackend"), 
                "Loading face recognition model: %s", rec_model.c_str());
    face_rec_net_ = cv::dnn::readNet(rec_model);
    RCLCPP_INFO(rclcpp::get_logger("OpenCVDNNBackend"), 
                "Face recognition model loaded successfully");
    
    face_models_loaded_ = true;
    
    // TODO: Load known faces database from a directory
    // For now, we'll just detect faces without recognition
    
    return true;
    
  } catch (const cv::Exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("OpenCVDNNBackend"),
                 "Failed to load face models: %s", e.what());
    face_models_loaded_ = false;
    return false;
  }
#else
  RCLCPP_ERROR(
    rclcpp::get_logger("OpenCVDNNBackend"),
    "OpenCV DNN backend not available");
  return false;
#endif
}

std::vector<FaceDetection> OpenCVDNNBackend::detect_faces(
  const cv::Mat& image,
  float det_threshold)
{
#ifdef USE_OPENCV_DNN
  std::vector<FaceDetection> faces;
  
  if (!face_models_loaded_) {
    RCLCPP_WARN_THROTTLE(
      rclcpp::get_logger("OpenCVDNNBackend"),
      *rclcpp::Clock::make_shared(),
      5000,
      "Face models not loaded");
    return faces;
  }
  
  try {
    // Prepare input for face detector (ResNet-SSD expects 300x300)
    cv::Mat blob = cv::dnn::blobFromImage(image, 1.0, cv::Size(300, 300),
                                          cv::Scalar(104.0, 177.0, 123.0),
                                          false, false);
    face_det_net_.setInput(blob);
    cv::Mat detections = face_det_net_.forward();
    
    // detections shape: [1, 1, N, 7]
    // Each detection: [img_id, class_id, confidence, x1, y1, x2, y2]
    cv::Mat detection_mat(detections.size[2], detections.size[3], CV_32F, detections.ptr<float>());
    
    for (int i = 0; i < detection_mat.rows; i++) {
      float confidence = detection_mat.at<float>(i, 2);
      
      if (confidence > det_threshold) {
        // Get bounding box (normalized coordinates)
        float x1 = detection_mat.at<float>(i, 3) * image.cols;
        float y1 = detection_mat.at<float>(i, 4) * image.rows;
        float x2 = detection_mat.at<float>(i, 5) * image.cols;
        float y2 = detection_mat.at<float>(i, 6) * image.rows;
        
        int left = std::max(0, static_cast<int>(x1));
        int top = std::max(0, static_cast<int>(y1));
        int width = std::min(static_cast<int>(x2 - x1), image.cols - left);
        int height = std::min(static_cast<int>(y2 - y1), image.rows - top);
        
        if (width > 0 && height > 0) {
          cv::Rect face_bbox(left, top, width, height);
          
          // Extract face ROI for recognition
          cv::Mat face_roi = image(face_bbox);
          
          // Get face embedding using ArcFace model
          cv::Mat face_blob = cv::dnn::blobFromImage(face_roi, 1.0/255.0, 
                                                      cv::Size(112, 112),
                                                      cv::Scalar(0, 0, 0),
                                                      true, false);
          face_rec_net_.setInput(face_blob);
          cv::Mat embedding = face_rec_net_.forward();
          
          // Convert embedding to vector for comparison
          std::vector<float> face_embedding(embedding.ptr<float>(), 
                                           embedding.ptr<float>() + embedding.total());
          
          // Find best match in known faces
          std::string best_name = "unknown";
          float best_similarity = 0.0f;
          
          for (const auto& known_face : known_faces_) {
            // Compute cosine similarity
            float similarity = cosine_similarity(face_embedding, known_face.second);
            if (similarity > best_similarity) {
              best_similarity = similarity;
              best_name = known_face.first;
            }
          }
          
          // Threshold for recognition (typical ArcFace threshold is ~0.3-0.4)
          if (best_similarity < 0.35f) {
            best_name = "unknown";
          }
          
          FaceDetection face;
          face.bbox = face_bbox;
          face.score = confidence;
          face.name = best_name;
          
          faces.push_back(face);
        }
      }
    }
    
  } catch (const cv::Exception& e) {
    RCLCPP_ERROR_THROTTLE(
      rclcpp::get_logger("OpenCVDNNBackend"),
      *rclcpp::Clock::make_shared(),
      5000,
      "Face detection error: %s", e.what());
  }
  
  return faces;
#else
  RCLCPP_ERROR(
    rclcpp::get_logger("OpenCVDNNBackend"),
    "OpenCV DNN backend not available");
  return std::vector<FaceDetection>();
#endif
}

#ifdef USE_OPENCV_DNN
float OpenCVDNNBackend::cosine_similarity(const std::vector<float>& a, const std::vector<float>& b)
{
  if (a.size() != b.size() || a.empty()) {
    return 0.0f;
  }
  
  float dot_product = 0.0f;
  float norm_a = 0.0f;
  float norm_b = 0.0f;
  
  for (size_t i = 0; i < a.size(); i++) {
    dot_product += a[i] * b[i];
    norm_a += a[i] * a[i];
    norm_b += b[i] * b[i];
  }
  
  float denominator = std::sqrt(norm_a) * std::sqrt(norm_b);
  if (denominator < 1e-8f) {
    return 0.0f;
  }
  
  return dot_product / denominator;
}

// Face database management methods
void OpenCVDNNBackend::add_known_face(const std::string& name, const std::vector<float>& embedding)
{
  known_faces_[name] = embedding;
}

bool OpenCVDNNBackend::remove_known_face(const std::string& name)
{
  return known_faces_.erase(name) > 0;
}

std::map<std::string, std::vector<float>> OpenCVDNNBackend::get_known_faces() const
{
  return known_faces_;
}

std::vector<float> OpenCVDNNBackend::extract_face_embedding(const cv::Mat& face_roi)
{
#ifdef USE_OPENCV_DNN
  if (!face_models_loaded_) {
    return {};
  }
  
  // Resize and preprocess face
  cv::Mat face_blob = cv::dnn::blobFromImage(face_roi, 1.0/255.0, cv::Size(112, 112), 
                                              cv::Scalar(0, 0, 0), true, false);
  
  face_rec_net_.setInput(face_blob);
  cv::Mat embedding_mat = face_rec_net_.forward();
  
  // Convert to vector
  std::vector<float> embedding(embedding_mat.ptr<float>(), 
                               embedding_mat.ptr<float>() + 512);
  
  return embedding;
#else
  return {};
#endif
}
#endif

cv::Scalar OpenCVDNNBackend::get_color_for_class(int class_id) const
{
  // Simple color generation for OpenCV DNN backend
  static const float golden_ratio = 0.618033988749895f;
  float hue = std::fmod(class_id * golden_ratio, 1.0f) * 179.0f;
  cv::Mat hsv(1, 1, CV_8UC3, cv::Scalar(hue, 200, 220));
  cv::Mat bgr;
  cv::cvtColor(hsv, bgr, cv::COLOR_HSV2BGR);
  return cv::Scalar(bgr.at<cv::Vec3b>(0, 0)[0], 
                    bgr.at<cv::Vec3b>(0, 0)[1], 
                    bgr.at<cv::Vec3b>(0, 0)[2]);
}

#ifdef USE_OPENCV_DNN
void OpenCVDNNBackend::process_tinyyolov2_output(
  const std::vector<cv::Mat>& outputs,
  const cv::Mat& image,
  float conf_threshold,
  float nms_threshold,
  std::vector<Detection>& detections)
{
  // TinyYOLOv2 output shape: [1, 125, 13, 13] for 416x416 input with 20 VOC classes
  // 125 = 5 anchors * (5 bbox params + 20 classes)
  // Format: grid cells contain 5 anchors each with [tx, ty, tw, th, objectness, class0...class19]
  // Need to apply sigmoid to tx, ty, objectness, class probs
  // Need to apply exp to tw, th and multiply by anchors
  
  if (outputs.empty()) {
    RCLCPP_ERROR(rclcpp::get_logger("OpenCVDNNBackend"), "No output from YOLO model");
    return;
  }
  
  const auto& output = outputs[0];
  
  // Verify output shape
  if (output.dims != 4 || output.size[1] != 125) {
    RCLCPP_ERROR(rclcpp::get_logger("OpenCVDNNBackend"), 
                 "Unexpected TinyYOLOv2 output shape: dims=%d, channels=%d", 
                 output.dims, output.size[1]);
    return;
  }
  
  int grid_h = output.size[2];  // 13
  int grid_w = output.size[3];  // 13
  int num_anchors = 5;
  int num_classes = 20;  // VOC classes
  
  // TinyYOLOv2 anchors (in grid cell units)
  float anchors[5][2] = {
    {1.08, 1.19},
    {3.42, 4.41},
    {6.63, 11.38},
    {9.42, 5.11},
    {16.62, 10.52}
  };
  
  float x_scale = image.cols / static_cast<float>(yolo_input_size_);
  float y_scale = image.rows / static_cast<float>(yolo_input_size_);
  
  std::vector<cv::Rect> boxes;
  std::vector<float> confidences;
  std::vector<int> class_ids;
  
  const float* data = (const float*)output.data;
  int grid_size = grid_h * grid_w;
  int channel_size = grid_size;
  
  // Process each grid cell
  for (int h = 0; h < grid_h; ++h) {
    for (int w = 0; w < grid_w; ++w) {
      int grid_idx = h * grid_w + w;
      
      // Each grid cell has 5 anchors
      for (int a = 0; a < num_anchors; ++a) {
        int channel_offset = a * (5 + num_classes);
        
        // Get raw values
        float tx = data[channel_offset * channel_size + 0 * channel_size + grid_idx];
        float ty = data[channel_offset * channel_size + 1 * channel_size + grid_idx];
        float tw = data[channel_offset * channel_size + 2 * channel_size + grid_idx];
        float th = data[channel_offset * channel_size + 3 * channel_size + grid_idx];
        float objectness_raw = data[channel_offset * channel_size + 4 * channel_size + grid_idx];
        
        // Apply sigmoid to objectness
        float objectness = 1.0f / (1.0f + std::exp(-objectness_raw));
        
        if (objectness >= conf_threshold) {
          // Apply sigmoid to tx, ty
          float bx = (1.0f / (1.0f + std::exp(-tx)) + w) / grid_w;
          float by = (1.0f / (1.0f + std::exp(-ty)) + h) / grid_h;
          
          // Apply exp to tw, th and multiply by anchors
          float bw = anchors[a][0] * std::exp(tw) / grid_w;
          float bh = anchors[a][1] * std::exp(th) / grid_h;
          
          // Find best class (apply sigmoid to class scores)
          int best_class_id = 0;
          float max_class_score = 0.0f;
          
          for (int c = 0; c < num_classes; ++c) {
            float class_raw = data[channel_offset * channel_size + (5 + c) * channel_size + grid_idx];
            float class_score = 1.0f / (1.0f + std::exp(-class_raw));
            if (class_score > max_class_score) {
              max_class_score = class_score;
              best_class_id = c;
            }
          }
          
          float confidence = objectness * max_class_score;
          
          if (confidence >= conf_threshold) {
            // Convert to image coordinates (bx, by, bw, bh are in [0, 1] range)
            int center_x = static_cast<int>(bx * image.cols);
            int center_y = static_cast<int>(by * image.rows);
            int bbox_width = static_cast<int>(bw * image.cols);
            int bbox_height = static_cast<int>(bh * image.rows);
            
            int left = center_x - bbox_width / 2;
            int top = center_y - bbox_height / 2;
            
            // Clamp to image boundaries
            left = std::max(0, std::min(left, image.cols - 1));
            top = std::max(0, std::min(top, image.rows - 1));
            bbox_width = std::min(bbox_width, image.cols - left);
            bbox_height = std::min(bbox_height, image.rows - top);
            
            if (bbox_width > 0 && bbox_height > 0) {
              boxes.push_back(cv::Rect(left, top, bbox_width, bbox_height));
              confidences.push_back(confidence);
              class_ids.push_back(best_class_id);
            }
          }
        }
      }
    }
  }
  
  // Apply NMS
  std::vector<int> indices;
  cv::dnn::NMSBoxes(boxes, confidences, conf_threshold, nms_threshold, indices);
  
  // Create final detections with VOC class names
  for (int idx : indices) {
    Detection det;
    det.bbox = boxes[idx];
    det.confidence = confidences[idx];
    det.class_id = class_ids[idx];
    
    // TinyYOLOv2 uses VOC dataset (20 classes), not COCO (80 classes)
    if (class_ids[idx] >= 0 && class_ids[idx] < static_cast<int>(VOC_CLASSES.size())) {
      det.label = VOC_CLASSES[class_ids[idx]];
    } else {
      det.label = "class_" + std::to_string(class_ids[idx]);
    }
    
    detections.push_back(det);
  }
}

void OpenCVDNNBackend::process_yolov5_output(
  const std::vector<cv::Mat>& outputs,
  const cv::Mat& image,
  float conf_threshold,
  float nms_threshold,
  std::vector<Detection>& detections)
{
  // YOLOv5 output shape: [1, 25200, 85] for 640x640 input with 80 classes
  // Format: [x_center, y_center, width, height, objectness, class0_score, ..., class79_score]
  
  if (outputs.empty()) {
    RCLCPP_ERROR(rclcpp::get_logger("OpenCVDNNBackend"), "No output from YOLO model");
    return;
  }
  
  std::vector<cv::Rect> boxes;
  std::vector<float> confidences;
  std::vector<int> class_ids;
  
  float x_factor = image.cols / static_cast<float>(yolo_input_size_);
  float y_factor = image.rows / static_cast<float>(yolo_input_size_);
  
  // Process each output
  for (size_t out_idx = 0; out_idx < outputs.size(); ++out_idx) {
    const auto& output = outputs[out_idx];
    
    // Debug output shape
    RCLCPP_INFO_ONCE(rclcpp::get_logger("OpenCVDNNBackend"),
                     "YOLOv5 output[%zu] shape: dims=%d, size=[%d, %d, %d]",
                     out_idx, output.dims, 
                     output.dims > 0 ? output.size[0] : 0,
                     output.dims > 1 ? output.size[1] : 0,
                     output.dims > 2 ? output.size[2] : 0);
    
    if (output.dims != 3) {
      RCLCPP_ERROR(rclcpp::get_logger("OpenCVDNNBackend"), 
                   "Unexpected output dims: %d (expected 3)", output.dims);
      continue;
    }
    
    const float* data = (float*)output.data;
    
    // Get dimensions - YOLOv5 output is [1, 25200, 85]
    int batch = output.size[0];
    int rows = output.size[1];  // Number of detections (e.g., 25200)
    int dimensions = output.size[2];  // 85 for COCO (4 bbox + 1 objectness + 80 classes)
    
    if (batch != 1) {
      RCLCPP_WARN(rclcpp::get_logger("OpenCVDNNBackend"), 
                  "Unexpected batch size: %d", batch);
    }
    
    for (int i = 0; i < rows; ++i) {
      const float* row = data + i * dimensions;
      
      float objectness = row[4];
      
      if (objectness >= conf_threshold) {
        // Find best class
        const float* class_scores = row + 5;
        int class_id = 0;
        float max_class_score = class_scores[0];
        
        for (int c = 1; c < dimensions - 5; ++c) {
          if (class_scores[c] > max_class_score) {
            max_class_score = class_scores[c];
            class_id = c;
          }
        }
        
        float confidence = objectness * max_class_score;
        
        if (confidence >= conf_threshold) {
          // Extract box coordinates (center format)
          float cx = row[0];
          float cy = row[1];
          float w = row[2];
          float h = row[3];
          
          // Convert to corner format and scale to image size
          int left = static_cast<int>((cx - w / 2.0f) * x_factor);
          int top = static_cast<int>((cy - h / 2.0f) * y_factor);
          int width = static_cast<int>(w * x_factor);
          int height = static_cast<int>(h * y_factor);
          
          // Clamp to image boundaries
          left = std::max(0, std::min(left, image.cols - 1));
          top = std::max(0, std::min(top, image.rows - 1));
          width = std::min(width, image.cols - left);
          height = std::min(height, image.rows - top);
          
          boxes.push_back(cv::Rect(left, top, width, height));
          confidences.push_back(confidence);
          class_ids.push_back(class_id);
        }
      }
    }
  }
  
  // Apply Non-Maximum Suppression
  std::vector<int> indices;
  cv::dnn::NMSBoxes(boxes, confidences, conf_threshold, nms_threshold, indices);
  
  // Create final detections
  for (int idx : indices) {
    Detection det;
    det.bbox = boxes[idx];
    det.confidence = confidences[idx];
    det.class_id = class_ids[idx];
    
    // Get class label
    if (class_ids[idx] >= 0 && class_ids[idx] < static_cast<int>(COCO_CLASSES.size())) {
      det.label = COCO_CLASSES[class_ids[idx]];
    } else {
      det.label = "class_" + std::to_string(class_ids[idx]);
    }
    
    detections.push_back(det);
  }
}

void OpenCVDNNBackend::process_yolov8_output(
  const std::vector<cv::Mat>& outputs,
  const cv::Mat& image,
  float conf_threshold,
  float nms_threshold,
  std::vector<Detection>& detections)
{
  // YOLOv8 has a different output format - this is a simplified implementation
  // YOLOv8 output shape is typically [1, 84, 8400] (transposed compared to v5)
  // Format: rows are [x_center, y_center, width, height, class0_score, ..., class79_score]
  
  std::vector<cv::Rect> boxes;
  std::vector<float> confidences;
  std::vector<int> class_ids;
  
  float x_factor = image.cols / static_cast<float>(yolo_input_size_);
  float y_factor = image.rows / static_cast<float>(yolo_input_size_);
  
  for (const auto& output : outputs) {
    // YOLOv8 typically outputs [1, 84, 8400]
    cv::Mat transposed;
    if (output.size[1] < output.size[2]) {
      // Need to transpose
      cv::Mat reshaped = output.reshape(1, output.size[1]);
      cv::transpose(reshaped, transposed);
    } else {
      transposed = output.reshape(1, output.size[1]);
    }
    
    const float* data = (float*)transposed.data;
    int rows = transposed.rows;
    int dimensions = transposed.cols;
    
    for (int i = 0; i < rows; ++i) {
      const float* row = data + i * dimensions;
      
      // YOLOv8 doesn't have separate objectness score
      const float* class_scores = row + 4;
      
      // Find best class
      int class_id = 0;
      float max_class_score = class_scores[0];
      
      for (int c = 1; c < dimensions - 4; ++c) {
        if (class_scores[c] > max_class_score) {
          max_class_score = class_scores[c];
          class_id = c;
        }
      }
      
      if (max_class_score >= conf_threshold) {
        // Extract box coordinates
        float cx = row[0];
        float cy = row[1];
        float w = row[2];
        float h = row[3];
        
        int left = static_cast<int>((cx - w / 2.0f) * x_factor);
        int top = static_cast<int>((cy - h / 2.0f) * y_factor);
        int width = static_cast<int>(w * x_factor);
        int height = static_cast<int>(h * y_factor);
        
        left = std::max(0, std::min(left, image.cols - 1));
        top = std::max(0, std::min(top, image.rows - 1));
        width = std::min(width, image.cols - left);
        height = std::min(height, image.rows - top);
        
        boxes.push_back(cv::Rect(left, top, width, height));
        confidences.push_back(max_class_score);
        class_ids.push_back(class_id);
      }
    }
  }
  
  // Apply NMS
  std::vector<int> indices;
  cv::dnn::NMSBoxes(boxes, confidences, conf_threshold, nms_threshold, indices);
  
  for (int idx : indices) {
    Detection det;
    det.bbox = boxes[idx];
    det.confidence = confidences[idx];
    det.class_id = class_ids[idx];
    
    if (class_ids[idx] >= 0 && class_ids[idx] < static_cast<int>(COCO_CLASSES.size())) {
      det.label = COCO_CLASSES[class_ids[idx]];
    } else {
      det.label = "class_" + std::to_string(class_ids[idx]);
    }
    
    detections.push_back(det);
  }
}
#endif

}  // namespace bootcamp_vision_cpp
