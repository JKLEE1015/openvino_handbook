#include <iostream>  // ��׼�����������
#include <string>    // �ַ��������
#include <vector>    // ��̬����������

#include <openvino/openvino.hpp> // OpenVINO��ͷ�ļ����������ѧϰģ������
#include <opencv2/opencv.hpp>    // OpenCV��ͷ�ļ�������ͼ��������ʾ

// ������ɫ���������ڻ��Ƽ���
std::vector<cv::Scalar> colors = { cv::Scalar(0, 0, 255) , cv::Scalar(0, 255, 0) , cv::Scalar(255, 0, 0) ,
                                   cv::Scalar(255, 100, 50) , cv::Scalar(50, 100, 255) , cv::Scalar(255, 50, 100) };

// �������������������ģ��Ԥ������ID��Ӧ
const std::vector<std::string> class_names = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
    "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
    "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
    "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
    "hair drier", "toothbrush" };

using namespace cv;
using namespace dnn;

// ����ͼ�����ű����ĺ���������ͼ��Ԥ����
Mat letterbox(const cv::Mat& source)
{
    int col = source.cols;
    int row = source.rows;
    int _max = MAX(col, row);
    Mat result = Mat::zeros(_max, _max, CV_8UC3);
    source.copyTo(result(Rect(0, 0, col, row)));
    return result;
}

int main(int argc, char* argv[])
{
    // ����1����ʼ��OpenVINO����ʱ����
    ov::Core core;

    // ����2������ģ��, �ɸ��������豸
    auto compiled_model = core.compile_model("yolov8s.xml", "CPU");

    // ����3��������������
    ov::InferRequest infer_request = compiled_model.create_infer_request();

    // ����4����ȡͼƬ�ļ�������Ԥ����
    Mat img = cv::imread("zidane.jpg");
    /// ���ֿ�߱�����
    Mat letterbox_img = letterbox(img);
    float scale = letterbox_img.size[0] / 640.0;
    Mat blob = blobFromImage(letterbox_img, 1.0 / 255.0, Size(640, 640), Scalar(), true);

    // ����5����������ͼ����������ģ�͵�����ڵ�
    auto input_port = compiled_model.input();
    ov::Tensor input_tensor(input_port.get_element_type(), input_port.get_shape(), blob.ptr(0));
    infer_request.set_input_tensor(input_tensor);

    // ����6����ʼ����
    infer_request.infer();

    // ����7����ȡ������
    auto output = infer_request.get_output_tensor(0);
    auto output_shape = output.get_shape();
    std::cout << "�����������״:" << output_shape << std::endl;
    int rows = output_shape[2];        //8400
    int dimensions = output_shape[1];  //84: box[cx, cy, w, h]+80 classes scores

    // ����8����������������
    float* data = output.data<float>();
    Mat output_buffer(output_shape[1], output_shape[2], CV_32F, data);
    transpose(output_buffer, output_buffer); // ת�þ����Ա����
    float score_threshold = 0.25;
    float nms_threshold = 0.5;
    std::vector<int> class_ids;
    std::vector<float> class_scores;
    std::vector<Rect> boxes;

    // ��ȡ�߽�����ID��������
    for (int i = 0; i < output_buffer.rows; i++) {
        Mat classes_scores = output_buffer.row(i).colRange(4, 84);
        Point class_id;
        double maxClassScore;
        minMaxLoc(classes_scores, 0, &maxClassScore, 0, &class_id);

        if (maxClassScore > score_threshold) {
            class_scores.push_back(maxClassScore);
            class_ids.push_back(class_id.x);
            float cx = output_buffer.at<float>(i, 0);
            float cy = output_buffer.at<float>(i, 1);
            float w = output_buffer.at<float>(i, 2);
            float h = output_buffer.at<float>(i, 3);

            int left = int((cx - 0.5 * w) * scale);
            int top = int((cy - 0.5 * h) * scale);
            int width = int(w * scale);
            int height = int(h * scale);

            boxes.push_back(Rect(left, top, width, height));
        }
    }
    // �Ǽ���ֵ���ƣ�ȥ���ص���
    std::vector<int> indices;
    NMSBoxes(boxes, class_scores, score_threshold, nms_threshold, indices);

    // �������յ�ͼ��
    for (size_t i = 0; i < indices.size(); i++) {
        int index = indices[i];
        int class_id = class_ids[index];
        rectangle(img, boxes[index], colors[class_id % 6], 2, 8);
        std::string label = class_names[class_id] + ":" + std::to_string(class_scores[index]).substr(0, 4);
        Size textSize = cv::getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, 0);
        Rect textBox(boxes[index].tl().x, boxes[index].tl().y - 15, textSize.width, textSize.height + 5);
        cv::rectangle(img, textBox, colors[class_id % 6], FILLED);
        putText(img, label, Point(boxes[index].tl().x, boxes[index].tl().y - 5), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255));
    }
    // ��ʾ���յ�ͼ��
    namedWindow("YOLOv8 OpenVINO ���� C++ ʾ��", WINDOW_AUTOSIZE);
    imshow("YOLOv8 OpenVINO ���� C++ ʾ��", img);
    waitKey(0);  // �ȴ�����
    destroyAllWindows(); // �ر����д���
    return 0;
}