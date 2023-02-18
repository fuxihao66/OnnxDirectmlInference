#include "../OnnxDMLCore/OperatorRegistration.h"

namespace ODI
{
class DmlOperatorUnsqueeze
{
public:
    DmlOperatorUnsqueeze(std::map<std::string, dml::Expression>& expressionMap, std::map<std::string, ONNX_PARSER::InitializerTensorInfo>& initializerMap, ONNX_PARSER::Op& node, dml::Graph& graph, unsigned int opsetVersion)
    {
        if (node.inputNames.size() != 1)
            throw std::exception("Unsqueeze parameter number must be 1!");
        auto & inputName = node.inputNames[0];

        CheckReference(initializerMap, inputName);

        if (expressionMap.count(inputName) == 0){
            throw std::exception("Dependency does not meet! Please check topological sorting!");
        }
        m_input = expressionMap[inputName];

        dml::TensorDimensions inputShape = m_input.GetOutputDesc().sizes;

        // TODO: handling scalar type 
        if (m_input.GetOutputDesc().sizes.size() == 1 && m_input.GetOutputDesc().sizes[0] == 1) {
            outputSizes.push_back(1);
        }
        else {
            // get unsqueeze axis from attribute 
            std::vector<int> axis;
            {
                //std::vector<char> temp;
                ONNX_PARSER::AttributeValWrapper attriWrapper = node.GetAttribute("axes", ONNX_PARSER::AttributeType::INTS);
                if (!attriWrapper.isValid())
                    assert(false);
                axis.resize(attriWrapper.getValue().size() / 4);
                memcpy(axis.data(), attriWrapper.getValue().data(), attriWrapper.getValue().size());
            }
            int AxisSize = axis.size();

            axis.push_back(AxisSize + inputShape.size());

            int index = 0;
            for (int i = 0; i < axis[0]; i++) {
                outputSizes.push_back(inputShape[index++]);
            }
            for (int i = 0; i < AxisSize; i++) {
                outputSizes.push_back(1);
                for (int j = axis[i] + 1; j < axis[i + 1]; j++) {
                    outputSizes.push_back(inputShape[index++]);
                }
            }
        }

        
        /*if (axis[AxisSize - 1] == AxisSize + inputShape.size() - 1 && AxisSize > 1)
            outputSizes.push_back(1);*/

    }

    dml::Expression Create(){
        // TODO: use identity to copy first?
        return dml::Reinterpret(m_input,
                                outputSizes,
                                std::nullopt);
    }
private:
    dml::TensorDimensions outputSizes;
    // DML_TENSOR_DATA_TYPE valueDataType;
    dml::Expression m_input;
    
};

DML_OP_DEFINE_CREATION_FUNCTION(Unsqueeze, DmlOperatorUnsqueeze);

} // namespace ODI