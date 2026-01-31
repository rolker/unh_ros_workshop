#ifndef BOOTCAMP_VISION_CPP__OPENCV_DNN_BACKEND_HPP_
#define BOOTCAMP_VISION_CPP__OPENCV_DNN_BACKEND_HPP_

#include "bootcamp_vision_cpp/inference_backend.hpp"

#ifdef USE_OPENCV_DNN
#include <opencv2/dnn.hpp>
#endif

namespace bootcamp_vision_cpp
{

class OpenCVDNNBackend : public InferenceBackend
{
public:
  OpenCVDNNBackend();
  ~OpenCVDNNBackend() override = default;
  
  std::string name() const override { return "OpenCV DNN"; }
  bool supports_cuda() const override { return false; }  // For now
  bool is_available() const override;
  
  bool load_yolo_model(const std::string& model_path) override;
  std::vector<Detection> detect_yolo(
    const cv::Mat& image,
    float conf_threshold,
    float nms_threshold) override;
  
  bool load_face_models(
    const std::string& det_model,
    const std::string& rec_model) override;
  std::vector<FaceDetection> detect_faces(
    const cv::Mat& image,
    float det_threshold) override;
  
  // Face database management
  void add_known_face(const std::string& name, const std::vector<float>& embedding) override;
  bool remove_known_face(const std::string& name) override;
  std::map<std::string, std::vector<float>> get_known_faces() const override;
  std::vector<float> extract_face_embedding(const cv::Mat& face_roi) override;
  
  cv::Scalar get_color_for_class(int class_id) const override;

private:
#ifdef USE_OPENCV_DNN
  cv::dnn::Net yolo_net_;
  cv::dnn::Net face_det_net_;
  cv::dnn::Net face_rec_net_;
  
  bool yolo_loaded_;
  bool face_models_loaded_;
  bool is_yolov5_;  // Track if loaded model is YOLOv5 (vs YOLOv8)
  int yolo_input_size_;  // Track input size
  
  // Face database: map of name -> face embedding (512-dim vector)
  std::map<std::string, std::vector<float>> known_faces_;
  
  // Helper function for face embedding similarity
  float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b);
  
  // Helper functions for YOLO output processing
  void process_tinyyolov2_output(
    const std::vector<cv::Mat>& outputs,
    const cv::Mat& image,
    float conf_threshold,
    float nms_threshold,
    std::vector<Detection>& detections);
  
  void process_yolov5_output(
    const std::vector<cv::Mat>& outputs,
    const cv::Mat& image,
    float conf_threshold,
    float nms_threshold,
    std::vector<Detection>& detections);
  
  void process_yolov8_output(
    const std::vector<cv::Mat>& outputs,
    const cv::Mat& image,
    float conf_threshold,
    float nms_threshold,
    std::vector<Detection>& detections);
#endif
};

}  // namespace bootcamp_vision_cpp

#endif  // BOOTCAMP_VISION_CPP__OPENCV_DNN_BACKEND_HPP_
