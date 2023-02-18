#pragma once



template <typename T>
class OperatorGenerator{
public:
    dml::Expression CreateDmlExpression(std::map<std::string, dml::Expression>& expressionMap, const ONNX_PARSER::Op& node, dml::Graph& graph, unsigned int opsetVersion){
        T op(expressionMap, node, graph, opsetVersion);

        return op.Create();
    }
};