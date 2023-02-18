#pragma once
#ifdef ONNXPARSER_EXPORTS
#define ONNXPARSER_API __declspec(dllexport)
#else
#define ONNXPARSER_API __declspec(dllimport)
#endif
namespace ONNX_PARSER {

	/*enum TensorProto_DataType : int {
		TensorProto_DataType_UNDEFINED = 0,
		TensorProto_DataType_FLOAT = 1,
		TensorProto_DataType_UINT8 = 2,
		TensorProto_DataType_INT8 = 3,
		TensorProto_DataType_UINT16 = 4,
		TensorProto_DataType_INT16 = 5,
		TensorProto_DataType_INT32 = 6,
		TensorProto_DataType_INT64 = 7,
		TensorProto_DataType_STRING = 8,
		TensorProto_DataType_BOOL = 9,
		TensorProto_DataType_FLOAT16 = 10,
		TensorProto_DataType_DOUBLE = 11,
		TensorProto_DataType_UINT32 = 12,
		TensorProto_DataType_UINT64 = 13,
		TensorProto_DataType_COMPLEX64 = 14,
		TensorProto_DataType_COMPLEX128 = 15,
		TensorProto_DataType_BFLOAT16 = 16,
		TensorProto_DataType_TensorProto_DataType_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<int32_t>::min(),
		TensorProto_DataType_TensorProto_DataType_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<int32_t>::max()
	};*/

	enum class ONNXPARSER_API PERROR {
		O_OK = 0,
		O_NOTFOUND
	};

	enum class ONNXPARSER_API AttributeType {
		UNDEFINED = 0,
		FLOAT = 1,
		INT = 2,
		STRING = 3,
		TENSOR = 4,
		GRAPH = 5,
		SPARSE_TENSOR = 11,
		TYPE_PROTO = 13,
		FLOATS = 6,
		INTS = 7,
		STRINGS = 8,
		TENSORS = 9,
		GRAPHS = 10,
		SPARSE_TENSORS = 12,
		TYPE_PROTOS = 14,
	};

	// Same as dml 0x5100 definition
	enum class ONNXPARSER_API TensorType {
		UKNOWN = 0,
		FLOAT,   // Default type, need to be casted to fp16 when upload resource to GPU
		FLOAT16,
		UINT32,
		UINT16,
		UINT8,
		INT32,
		INT16,
		INT8,
		DOUBLE,
		UINT64,
		INT64,
	};


}
