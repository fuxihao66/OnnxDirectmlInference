//
// Created by daquexian on 8/3/18.
// Modified by fuxihao on 10/7/2022
//

#pragma once
#include "onnx.proto3.pb.h"
#include <vector>
#include <string>
#include <optional>
#include "Common.h"
 
namespace ONNX_PARSER {
    class ONNXPARSER_API NodeAttrHelper {
    public:
        NodeAttrHelper(const NodeAttrHelper& old);
        explicit NodeAttrHelper(onnx::NodeProto proto);

        /*float get(const std::string& key,
            const float def_val) const;
        int get(const std::string& key,
            const int def_val) const;*/


        bool get(const std::string& key, std::string& ret) const;
        bool get(const std::string& key, float& ret) const;
        bool get(const std::string& key, int& ret) const;
        bool get(const std::string& key, std::vector<float>& ret) const;
        bool get(const std::string& key, std::vector<int>& ret) const;

        bool get(const std::string& key, onnx::TensorProto& ret) const;
        bool get(const std::string& key, onnx::SparseTensorProto& ret) const;


        /*std::vector<float> get(const std::string& key,
            const std::vector<float>& def_val) const;
        std::vector<int> get(const std::string& key,
            const std::vector<int>& def_val) const;
        std::string get(const std::string& key,
            const std::string& def_val) const;*/

        bool has_attr(const std::string& key) const;

    private:
        onnx::NodeProto node_;
    };

}


/**
 * Wrapping onnx::NodeProto for retrieving attribute values
 */

