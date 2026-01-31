#include <chrono>
#include <memory>
#include <opencv2/opencv.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.h>

using namespace std::chrono_literals;

class CameraPublisher : public rclcpp::Node
{
public:
  CameraPublisher()
  : Node("camera_publisher")
  {
    this->declare_parameter("cam_id", 0);
    this->declare_parameter("width", 0.0);
    this->declare_parameter("height", 0.0);
    this->declare_parameter("fps", 30.0);

    int cam_id = this->get_parameter("cam_id").as_int();
    
    cap_ = std::make_unique<cv::VideoCapture>(cam_id);
    if (!cap_->isOpened()) {
      RCLCPP_ERROR(this->get_logger(), "Could not open camera id %d. Try --ros-args -p cam_id:=1", cam_id);
      throw std::runtime_error("Failed to open camera");
    }

    double w = cap_->get(cv::CAP_PROP_FRAME_WIDTH);
    double h = cap_->get(cv::CAP_PROP_FRAME_HEIGHT);
    double fps = cap_->get(cv::CAP_PROP_FPS);
    
    RCLCPP_INFO(this->get_logger(), "Camera default settings: %.0fx%.0f @ %.1ffps", w, h, fps);

    double param_w = this->get_parameter("width").as_double();
    double param_h = this->get_parameter("height").as_double();
    double param_fps = this->get_parameter("fps").as_double();

    if (param_w > 0) w = param_w;
    if (param_h > 0) h = param_h;
    if (param_fps > 0) fps = param_fps;

    publisher_ = this->create_publisher<sensor_msgs::msg::Image>("/camera/image_raw", 10);
    
    int period_ms = static_cast<int>(1000.0 / std::max(fps, 1.0));
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(period_ms),
      std::bind(&CameraPublisher::timer_callback, this));

    RCLCPP_INFO(this->get_logger(), 
                "Publishing /camera/image_raw from cam_id=%d (%.0fx%.0f @ ~%.1ffps)", 
                cam_id, w, h, fps);
  }

  ~CameraPublisher()
  {
    if (cap_ && cap_->isOpened()) {
      cap_->release();
    }
  }

private:
  void timer_callback()
  {
    cv::Mat frame;
    bool ok = cap_->read(frame);
    
    if (!ok || frame.empty()) {
      RCLCPP_WARN(this->get_logger(), "Frame read failed.");
      return;
    }

    std_msgs::msg::Header header;
    header.stamp = this->now();
    header.frame_id = "camera";
    
    auto msg = cv_bridge::CvImage(header, "bgr8", frame).toImageMsg();
    publisher_->publish(*msg);
  }

  std::unique_ptr<cv::VideoCapture> cap_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<CameraPublisher>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
