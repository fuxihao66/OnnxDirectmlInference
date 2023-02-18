

class Network{
public:
    Network() = default;
    Network(const std::wstring& onnx_file_path, const std::string& model_name = "");
    ~Network();
    bool CreateModelAndUploadData(FRDGBuilder& GraphBuilder, const std::wstring& onnx_file_path, const std::string& model_name);
    bool ExecuteInference(FRDGBuilder& GraphBuilder, std::map<std::string, FRDGTextureRef> InputBuffers, FRDGTextureRef OutputBuffer);
private:
    const std::string m_model_name;
    const std::wstring m_onnx_file_path;
    ODIRHI* m_ODIRHIExtensions;
};