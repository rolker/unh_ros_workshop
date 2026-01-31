#ifndef BOOTCAMP_VISION_CPP__INFERENCE_BACKEND_HPP_
#define BOOTCAMP_VISION_CPP__INFERENCE_BACKEND_HPP_

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace bootcamp_vision_cpp
{

// Detection result structures
struct Detection
{
  cv::Rect bbox;
  std::string label;
  float confidence;
  int class_id;
};

struct FaceDetection
{
  cv::Rect bbox;
  std::string name;
  float score;
  std::vector<float> embedding;  // 512D for ArcFace
};

// Abstract base class for inference backends
class InferenceBackend
{
public:
  virtual ~InferenceBackend() = default;
  
  // Backend identification
  virtual std::string name() const = 0;
  virtual bool supports_cuda() const = 0;
  virtual bool is_available() const = 0;
  
  // YOLO operations
  virtual bool load_yolo_model(const std::string& model_path) = 0;
  virtual std::vector<Detection> detect_yolo(
    const cv::Mat& image,
    float conf_threshold = 0.35,
    float nms_threshold = 0.45) = 0;
  
  // Face operations
  virtual bool load_face_models(
    const std::string& det_model,
    const std::string& rec_model) = 0;
  virtual std::vector<FaceDetection> detect_faces(
    const cv::Mat& image,
    float det_threshold = 0.5) = 0;
  
  // Face database management
  virtual void add_known_face(const std::string& name, const std::vector<float>& embedding) = 0;
  virtual bool remove_known_face(const std::string& name) = 0;
  virtual std::map<std::string, std::vector<float>> get_known_faces() const = 0;
  virtual std::vector<float> extract_face_embedding(const cv::Mat& face_roi) = 0;
  
  // Utility function for color generation
  virtual cv::Scalar get_color_for_class(int class_id) const = 0;
};

// Factory function to create appropriate backend
std::unique_ptr<InferenceBackend> create_backend(
  const std::string& backend_name = "auto");

// Get list of available backends
std::vector<std::string> get_available_backends();

}  // namespace bootcamp_vision_cpp

#endif  // BOOTCAMP_VISION_CPP__INFERENCE_BACKEND_HPP_
