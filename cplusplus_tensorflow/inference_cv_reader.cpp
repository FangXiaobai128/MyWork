// 这段代码读取图片使用了opencv三方库，在另外的inference_tf_reader.cpp中则是使用的纯tensorflow c++ api
// 修改的内容主要是整理了部分代码，因为第一版有些混乱
// Original by @TerryBryant
// First created in Mar. 28th, 2018
// modified in Apr. 2nd, 2018
#define COMPILER_MSVC
#define NOMINMAX
#define PLATFORM_WINDOWS   // 指定使用tensorflow/core/platform/windows/cpu_info.h

#include <iostream>
#include <opencv2/opencv.hpp>   // 使用opencv读取图片
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"

using namespace tensorflow;
using std::cout;
using std::endl;


int main()
{
	const std::string model_path = "E:/ProjectPython/prj_tensorflow/yzm/frozen_model_12000.pb";// tensorflow模型文件，注意不能含有中文
	const std::string image_path = "captcha/00e7.png";    // 待inference的图片
	const std::string input_name = "x_input";    // 模型文件中的输入名
	const std::string output_name = "x_predict";    // 模型文件中的输出名
	const int input_height = 34;  // 模型文件的输入图片尺寸
	const int input_width = 66;
	const float input_mean = 128.0;  // 输入图片预测里中的均值和标准差
	const float input_std = 128.0;


	// 设置输入图像数据
	//利用opencv读取图片
	cv::Mat img = cv::imread(image_path);
	cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
	int channels = img.channels();  // 本模型的输入图片是单通道的

	// 图像预处理
	img.convertTo(img, CV_32F);
	cv::resize(img, img, cv::Size(input_width, input_height), cv::INTER_LINEAR);
	img = (img - input_mean) / input_std;	

	// 取图像数据，赋给tensorflow支持的Tensor变量中
	const float* source_data = (float*)img.data;
	tensorflow::Tensor input_tensor(DT_FLOAT, TensorShape({ 1, input_height, input_width, channels })); //这里只输入一张图片，参考tensorflow的数据格式NHWC
	auto input_tensor_mapped = input_tensor.tensor<float, 4>(); // input_tensor_mapped相当于input_tensor的数据接口，“4”表示数据是4维的
																// 后面取出最终结果时也能看到这种用法

	// 把数据复制到input_tensor_mapped中，实际上就是遍历opencv的Mat数据
	for (int i = 0; i < input_height; i++) {
		const float* source_row = source_data + (i * input_width * channels);
		for (int j = 0; j < input_width; j++) {
			const float* source_pixel = source_row + (j * channels);
			for (int c = 0; c < channels; c++) {
				const float* source_value = source_pixel + c;
				input_tensor_mapped(0, i, j, c) = *source_value;
			}
		}
	}



	// 读取二进制的模型文件到graph中
	GraphDef graph_def;
	TF_CHECK_OK(ReadBinaryProto(Env::Default(), model_path, &graph_def));
	//status = ReadBinaryProto(Env::Default(), model_path, &graph_def);
	//if (!status.ok()) {
	//	std::cerr << status.ToString() << endl;
	//	return -1;
	//}
	//else {
	//	cout << "Load graph protobuf successfully" << endl;
	//}



	// 初始化tensorflow session
    std::unique_ptr<tensorflow::Session> session(
		tensorflow::NewSession(tensorflow::SessionOptions()));
	//Session* session;	
	//TF_CHECK_OK(tensorflow::NewSession(SessionOptions(), &session));
	//tensorflow::SessionOptions sess_opt;
	//sess_opt.config.mutable_gpu_options()->set_per_process_gpu_memory_fraction(0.5); // 设置占用显存比例，默认全部占用
	//sess_opt.config.mutable_gpu_options()->set_allow_growth(true);
	//(&session)->reset(tensorflow::NewSession(sess_opt));

	//Session* session;
	//Status status = NewSession(SessionOptions(), &session);
	//if (!status.ok()){
	//	std::cerr << status.ToString() << endl;
	//	return -1;
	//}
	//else {
	//	cout << "Session created successfully" << endl;
	//}	



	// 将graph加载到session
	TF_CHECK_OK(session->Create(graph_def));
	//status = session->Create(graph_def);
	//if (!status.ok()) {
	//	std::cerr << status.ToString() << endl;
	//	return -1;
	//}
	//else {
	//	cout << "Add graph to session successfully" << endl;
	//}
	

	// 输入，模型用到了dropout，所以这里有个“keep_prob”
	tensorflow::Tensor keep_prob(tensorflow::DT_FLOAT, TensorShape());
	keep_prob.scalar<float>()() = 1.0;
	std::vector<std::pair<std::string, tensorflow::Tensor>> inputs = {		
		{ input_name, input_tensor },
		{ "keep_prob", keep_prob },
	};

	// 输出outputs
	std::vector<tensorflow::Tensor> outputs;

	// 运行会话，计算输出output_name，即我在模型中定义的输出数据名称，最终结果保存在outputs中
	TF_CHECK_OK(session->Run(inputs, { output_name }, {}, &outputs));
	//status = session->Run(inputs, { output_name }, {}, &outputs);
	//if (!status.ok()) {
	//	std::cerr << status.ToString() << endl;
	//	return -1;
	//}
	//else {
	//	cout << "Run session successfully" << endl;
	//}

	// 下面进行输出结果的可视化
	tensorflow::Tensor output = std::move(outputs.at(0)); // 模型只输出一个结果，这里首先把结果移出来（也为了更好的展示）
	auto out_shape = output.shape(); // 这里的输出结果为1x4x16
	auto out_val = output.tensor<float, 3>(); // 与开头的用法对应，3代表结果的维度
	// cout << out_val.argmax(2) << " "; // 预测结果，与python一致，但具体数值有差异，猜测是计算精度不同造成的

	// 输出这个1x4x16的矩阵（实际就是4x16）
	for (int i = 0; i < out_shape.dim_size(1); i++) {
		for (int j = 0; j < out_shape.dim_size(2); j++) {
			cout << out_val(0, i, j) << " ";
		}
		cout << endl;
	}

	return 1;
}
