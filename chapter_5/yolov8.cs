// ����OpenCVSharp���ѧϰ���������ܡ�OpenVinoSharp��չ������������OpenVinoSharp���������ռ�
using OpenCvSharp.Dnn;
using OpenCvSharp;
using OpenVinoSharp.Extensions.process;
using OpenVinoSharp.Extensions.result;
using OpenVinoSharp;

// ����yolov8_det�����ռ��µĳ�����
namespace yolov8_det
{
    // �ڲ���Program��������ڵ�
    internal class Program
    {
        // ������������ִ�����
        static void Main(string[] args)
        {
            // ������Ƶ�ļ�·����ģ���ļ�·��
            string video_path = "E:\\ModelData\\image\\bus.jpg";
            string model_path = "E:\\Model\\yolo\\yolov8s.onnx";

            // ��ʼ��OpenVINO���Ķ���
            Core core = new Core();
            // ��ȡģ��
            Model model = core.read_model(model_path);
            // ����ģ������CPU������
            CompiledModel compiled_model = core.compile_model(model, "CPU");
            // ���������������
            InferRequest request = compiled_model.create_infer_request();

            // ��ȡͼ��
            Mat img = Cv2.ImRead(video_path);
            // ��ʼ����������
            float factor = 0.0f;
            // ͼ��Ԥ�������Ԥ���������ݺ���������
            float[] input_data = preprocess(img, out factor);
            // ��Ԥ�������������������������������
            request.get_input_tensor().set_data(input_data);

            // ִ��ģ������
            request.infer();
            // ��ȡ������
            float[] output_data = request.get_output_tensor().get_data<float>(8400 * 84);
            // �������������õ������
            DetResult result = postprocess(output_data, factor);
            // ���������ʾ��ԭͼ��
            Mat res_mat = Visualize.draw_det_result(result, img);
            // ��ʾ���ͼ��
            Cv2.ImShow("Result", res_mat);
            // �ȴ������¼������ڹر�����
            Cv2.WaitKey(0);
        }

        // ͼ��Ԥ��������������ɫ�ռ�ת���������ߴ硢��һ����ά������
        public static float[] preprocess(Mat img, out float factor)
        {
            Mat mat = new Mat();
            // ��BGRͼ��ת��ΪRGBͼ��
            Cv2.CvtColor(img, mat, ColorConversionCodes.BGR2RGB);
            // ����ͼ��ߴ粢��¼��������
            mat = Resize.letterbox_img(mat, 640, out factor);
            // ���ݹ�һ��
            mat = Normalize.run(mat, true);
            // �ı�����ά��˳����ƥ��ģ������Ҫ��
            return Permute.run(mat);
        }

        // �����������ɸѡ���Ǽ���ֵ����(NMS)����֯�����
        public static DetResult postprocess(float[] result, float factor)
        {
            // ��ʼ���߽�����ID�����Ŷ��б�
            List<Rect> positionBoxes = new List<Rect>();
            List<int> classIds = new List<int>();
            List<float> confidences = new List<float>();

            // ��������������ȡ��Ч����
            for (int i = 0; i < 8400; i++)
            {
                for (int j = 4; j < 84; j++)
                {
                    float source = result[8400 * j + i];
                    int label = j - 4;
                    // �����Ԥ��ĵ÷ִ�����ֵ
                    if (source > 0.2)
                    {
                        float maxSource = source;
                        float cx = result[8400 * 0 + i]; // ����x����
                        float cy = result[8400 * 1 + i]; // ����y����
                        float ow = result[8400 * 2 + i]; // Ԥ����
                        float oh = result[8400 * 3 + i]; // Ԥ��߶�
                        // ����ʵ�ʱ߽��λ��
                        int x = (int)((cx - 0.5 * ow) * factor);
                        int y = (int)((cy - 0.5 * oh) * factor);
                        int width = (int)(ow * factor);
                        int height = (int)(oh * factor);
                        Rect box = new Rect(x, y, width, height);
                        // ������Ч�����������Ŷ�
                        positionBoxes.Add(box);
                        classIds.Add(label);
                        confidences.Add(maxSource);
                    }
                }
            }
            // ����DetResultʵ�����ڴ洢���ռ����
            DetResult re = new DetResult();
            // ����һ��������������NMS��������Ч��
            int[] indexes = new int[positionBoxes.Count];
            // Ӧ�÷Ǽ���ֵ���ƣ�ɸѡ�ص���
            CvDnn.NMSBoxes(positionBoxes, confidences, 0.2f, 0.5f, out indexes);
            // ����NMS�����װ���յ�DetResult����
            for (int i = 0; i < indexes.Length; i++)
            {
                int index = indexes[i];
                re.add(classIds[index], confidences[index], positionBoxes[index]);
            }
            // ���ش����ļ����
            return re;
        }
    }
}