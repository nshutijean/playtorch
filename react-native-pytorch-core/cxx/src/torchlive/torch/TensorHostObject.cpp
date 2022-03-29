/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <c10/util/Optional.h>

#include "TensorHostObject.h"
#include "utils/constants.h"
#include "utils/helpers.h"

// Namespace alias for torch to avoid namespace conflicts with torchlive::torch
namespace torch_ = torch;

namespace torchlive {
namespace torch {

// TensorHostObject Method Names
static const std::string ABS = "abs";
static const std::string ADD = "add";
static const std::string ARGMAX = "argmax";
static const std::string DIV = "div";
static const std::string MUL = "mul";
static const std::string PERMUTE = "permute";
static const std::string SIZE = "size";
static const std::string SOFTMAX = "softmax";
static const std::string SQUEEZE = "squeeze";
static const std::string SUB = "sub";
static const std::string TOPK = "topk";
static const std::string TOSTRING = "toString";
static const std::string UNSQUEEZE = "unsqueeze";

// TensorHostObject Property Names
static const std::string DATA = "data";
static const std::string DTYPE = "dtype";
static const std::string SHAPE = "shape";

// TensorHostObject Properties
static const std::vector<std::string> PROPERTIES = {DATA, DTYPE, SHAPE};

// TensorHostObject Methods
static const std::vector<std::string> METHODS = {
    ABS,
    ADD,
    ARGMAX,
    DIV,
    MUL,
    PERMUTE,
    SIZE,
    SOFTMAX,
    SQUEEZE,
    SUB,
    TOPK,
    TOSTRING,
    UNSQUEEZE};

using namespace facebook;

TensorHostObject::TensorHostObject(jsi::Runtime& runtime, torch_::Tensor t)
    : abs_(createAbs(runtime)),
      add_(createAdd(runtime)),
      argmax_(createArgmax(runtime)),
      div_(createDiv(runtime)),
      mul_(createMul(runtime)),
      permute_(createPermute(runtime)),
      size_(createSize(runtime)),
      softmax_(createSoftmax(runtime)),
      squeeze_(createSqueeze(runtime)),
      sub_(createSub(runtime)),
      topk_(createTopK(runtime)),
      toString_(createToString(runtime)),
      unsqueeze_(createUnsqueeze(runtime)),
      tensor(t) {}

TensorHostObject::~TensorHostObject() {}

std::vector<jsi::PropNameID> TensorHostObject::getPropertyNames(
    jsi::Runtime& runtime) {
  std::vector<jsi::PropNameID> result;
  for (std::string property : PROPERTIES) {
    result.push_back(jsi::PropNameID::forUtf8(runtime, property));
  }
  for (std::string method : METHODS) {
    result.push_back(jsi::PropNameID::forUtf8(runtime, method));
  }
  return result;
}

jsi::Value TensorHostObject::get(
    jsi::Runtime& runtime,
    const jsi::PropNameID& propNameId) {
  auto name = propNameId.utf8(runtime);

  if (name == ABS) {
    return jsi::Value(runtime, abs_);
  } else if (name == ADD) {
    return jsi::Value(runtime, add_);
  } else if (name == ARGMAX) {
    return jsi::Value(runtime, argmax_);
  } else if (name == DATA) {
    auto tensor = this->tensor;
    int byteLength = tensor.nbytes();
    auto type = tensor.dtype();

    // TODO(T113480543): enable BigInt data view once Hermes support the
    // BigIntArray
    if (type == torch_::kInt64) {
      throw jsi::JSError(
          runtime, "the property 'data' of BigInt Tensor is not supported.");
    }

    jsi::ArrayBuffer buffer = runtime.global()
                                  .getPropertyAsFunction(runtime, "ArrayBuffer")
                                  .callAsConstructor(runtime, byteLength)
                                  .asObject(runtime)
                                  .getArrayBuffer(runtime);

    std::memcpy(buffer.data(runtime), tensor.data_ptr(), byteLength);

    std::string typedArrayName;
    if (type == torch_::kUInt8) {
      typedArrayName = "Uint8Array";
    } else if (type == torch_::kInt8) {
      typedArrayName = "Int8Array";
    } else if (type == torch_::kInt16) {
      typedArrayName = "Int16Array";
    } else if (type == torch_::kInt32) {
      typedArrayName = "Int32Array";
    } else if (type == torch_::kFloat32) {
      typedArrayName = "Float32Array";
    } else if (type == torch_::kFloat64) {
      typedArrayName = "Float64Array";
    } else {
      throw jsi::JSError(runtime, "tensor data dtype is not supported");
    }

    return runtime.global()
        .getPropertyAsFunction(runtime, typedArrayName.c_str())
        .callAsConstructor(runtime, buffer)
        .asObject(runtime);
  } else if (name == DIV) {
    return jsi::Value(runtime, div_);
  } else if (name == DTYPE) {
    return jsi::String::createFromUtf8(
        runtime,
        utils::constants::getStringFromDtype(
            caffe2::typeMetaToScalarType(this->tensor.dtype())));
  } else if (name == MUL) {
    return jsi::Value(runtime, mul_);
  } else if (name == PERMUTE) {
    return jsi::Value(runtime, permute_);
  } else if (name == SHAPE) {
    return this->size_.call(runtime);
  } else if (name == SIZE) {
    return jsi::Value(runtime, size_);
  } else if (name == SOFTMAX) {
    return jsi::Value(runtime, softmax_);
  } else if (name == SQUEEZE) {
    return jsi::Value(runtime, squeeze_);
  } else if (name == SUB) {
    return jsi::Value(runtime, sub_);
  } else if (name == TOPK) {
    return jsi::Value(runtime, topk_);
  } else if (name == TOSTRING) {
    return jsi::Value(runtime, toString_);
  } else if (name == UNSQUEEZE) {
    return jsi::Value(runtime, unsqueeze_);
  }

  int idx = -1;
  try {
    idx = std::stoi(name.c_str());
  } catch (const std::exception& e) {
    // Cannot parse name value to int. This can happen when the name in bracket
    // or dot notion is not an int (e.g., tensor['foo']).
    // Let's ignore this exception here since this function will return
    // undefined if it reaches the function end.
  }
  // Check if index is within bounds of dimension 0
  if (idx >= 0 && idx < this->tensor.size(0)) {
    auto outputTensor = this->tensor.index({idx});
    auto tensorHostObject =
        std::make_shared<torchlive::torch::TensorHostObject>(
            runtime, std::move(outputTensor));
    return jsi::Object::createFromHostObject(runtime, tensorHostObject);
  }

  return jsi::Value::undefined();
}

jsi::Function TensorHostObject::createAbs(jsi::Runtime& runtime) {
  auto absFunc = [this](
                     jsi::Runtime& runtime,
                     const jsi::Value& thisValue,
                     const jsi::Value* arguments,
                     size_t count) -> jsi::Value {
    if (count > 0) {
      throw jsi::JSError(
          runtime,
          "0 argument is expected but " + std::to_string(count) +
              " are given.");
    }
    auto outputTensor = this->tensor.abs();
    auto tensorHostObject =
        std::make_shared<torchlive::torch::TensorHostObject>(
            runtime, outputTensor);
    return jsi::Object::createFromHostObject(runtime, tensorHostObject);
  };
  return jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, TOSTRING), 0, absFunc);
}

jsi::Function TensorHostObject::createAdd(jsi::Runtime& runtime) {
  auto addFunc = [this](
                     jsi::Runtime& runtime,
                     const jsi::Value& thisValue,
                     const jsi::Value* arguments,
                     size_t count) {
    if (count < 1) {
      throw jsi::JSError(runtime, "At least 1 arg required");
    }
    auto alphaValue = utils::helpers::parseKeywordArgument(
        runtime, arguments, 1, count, "alpha");
    auto alphaScalar = alphaValue.isUndefined()
        ? at::Scalar(1)
        : at::Scalar(alphaValue.asNumber());
    auto tensor = this->tensor;
    if (arguments[0].isNumber()) {
      auto value = arguments[0].asNumber();
      tensor = tensor.add(value, alphaScalar);
    } else {
      auto otherTensorHostObject =
          utils::helpers::parseTensor(runtime, &arguments[0]);
      auto otherTensor = otherTensorHostObject->tensor;
      tensor = tensor.add(otherTensor, alphaScalar);
    }
    auto tensorHostObject =
        std::make_shared<torchlive::torch::TensorHostObject>(runtime, tensor);
    return jsi::Object::createFromHostObject(runtime, tensorHostObject);
  };
  return jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, ADD), 1, addFunc);
}

jsi::Function TensorHostObject::createArgmax(jsi::Runtime& runtime) {
  auto argmaxImpl = [this](
                        jsi::Runtime& runtime,
                        const jsi::Value& thisValue,
                        const jsi::Value* arguments,
                        size_t count) {
    auto tensor = this->tensor;
    auto max = tensor.argmax();
    return jsi::Value(max.item<int>());
  };

  return jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, ARGMAX), 0, argmaxImpl);
}

jsi::Function TensorHostObject::createToString(jsi::Runtime& runtime) {
  auto toStringFunc = [this](
                          jsi::Runtime& runtime,
                          const jsi::Value& thisValue,
                          const jsi::Value* arguments,
                          size_t count) -> jsi::Value {
    auto tensor = this->tensor;
    std::ostringstream stream;
    stream << tensor;
    std::string tensor_string = stream.str();
    auto val = jsi::String::createFromUtf8(runtime, tensor_string);
    return jsi::Value(std::move(val));
  };
  return jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, TOSTRING), 0, toStringFunc);
}

jsi::Function TensorHostObject::createDiv(jsi::Runtime& runtime) {
  auto divFunc = [this](
                     jsi::Runtime& runtime,
                     const jsi::Value& thisValue,
                     const jsi::Value* arguments,
                     size_t count) -> jsi::Value {
    if (count < 1) {
      throw jsi::JSError(runtime, "At least 1 arg required");
    }

    auto roundingModeValue = utils::helpers::parseKeywordArgument(
        runtime, arguments, 1, count, "rounding_mode");

    c10::optional<c10::string_view> roundingMode;
    if (!roundingModeValue.isUndefined()) {
      roundingMode = roundingModeValue.asString(runtime).utf8(runtime);
    }

    auto tensor = this->tensor;
    if (arguments[0].isNumber()) {
      auto value = arguments[0].asNumber();
      tensor = tensor.div(value, roundingMode);
    } else {
      auto otherTensorHostObject =
          utils::helpers::parseTensor(runtime, &arguments[0]);
      auto otherTensor = otherTensorHostObject->tensor;
      tensor = tensor.div(otherTensor, roundingMode);
    }
    auto tensorHostObject =
        std::make_shared<torchlive::torch::TensorHostObject>(runtime, tensor);
    return jsi::Object::createFromHostObject(runtime, tensorHostObject);
  };
  return jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, DIV), 1, divFunc);
}

jsi::Function TensorHostObject::createMul(jsi::Runtime& runtime) {
  auto mulFunc = [this](
                     jsi::Runtime& runtime,
                     const jsi::Value& thisValue,
                     const jsi::Value* arguments,
                     size_t count) {
    if (count < 1) {
      throw jsi::JSError(runtime, "At least 1 arg required");
    }

    auto tensor = this->tensor;
    if (arguments[0].isNumber()) {
      auto value = arguments[0].asNumber();
      tensor = tensor.mul(value);
    } else {
      auto otherTensorHostObject =
          utils::helpers::parseTensor(runtime, &arguments[0]);
      auto otherTensor = otherTensorHostObject->tensor;
      tensor = tensor.mul(otherTensor);
    }
    auto tensorHostObject =
        std::make_shared<torchlive::torch::TensorHostObject>(runtime, tensor);

    return jsi::Object::createFromHostObject(runtime, tensorHostObject);
  };

  return jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, MUL), 1, mulFunc);
}

jsi::Function TensorHostObject::createPermute(jsi::Runtime& runtime) {
  auto permuteImpl = [this](
                         jsi::Runtime& runtime,
                         const jsi::Value& thisValue,
                         const jsi::Value* arguments,
                         size_t count) {
    if (count != 1) {
      throw jsi::JSError(runtime, "dimensions argument is required");
    }

    std::vector<int64_t> dims = {};
    int nextArgIndex =
        utils::helpers::parseSize(runtime, arguments, 0, count, &dims);

    auto tensor = this->tensor;
    tensor = tensor.permute(dims);
    auto permutedTensorHostObject =
        std::make_shared<torchlive::torch::TensorHostObject>(runtime, tensor);
    return jsi::Object::createFromHostObject(runtime, permutedTensorHostObject);
  };

  return jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, PERMUTE), 1, permuteImpl);
}

jsi::Function TensorHostObject::createSize(jsi::Runtime& runtime) {
  auto sizeFunc = [this](
                      jsi::Runtime& runtime,
                      const jsi::Value& thisValue,
                      const jsi::Value* arguments,
                      size_t count) -> jsi::Value {
    auto tensor = this->tensor;
    torch_::IntArrayRef dims = tensor.sizes();
    jsi::Array jsShape = jsi::Array(runtime, dims.size());
    for (int i = 0; i < dims.size(); i++) {
      jsShape.setValueAtIndex(runtime, i, jsi::Value((int)dims[i]));
    }

    return jsShape;
  };
  return jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, SIZE), 0, sizeFunc);
}

jsi::Function TensorHostObject::createSoftmax(jsi::Runtime& runtime) {
  auto softmaxFunc = [this](
                         jsi::Runtime& runtime,
                         const jsi::Value& thisValue,
                         const jsi::Value* arguments,
                         size_t count) {
    if (count < 1) {
      throw jsi::JSError(runtime, "This function requires at least 1 argument");
    }
    auto dimension = arguments[0].asNumber();
    auto resultTensor = this->tensor.softmax(dimension);
    auto outputTensorHostObject =
        std::make_shared<torchlive::torch::TensorHostObject>(
            runtime, resultTensor);
    return jsi::Object::createFromHostObject(runtime, outputTensorHostObject);
  };

  return jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, SOFTMAX), 1, softmaxFunc);
}

jsi::Function TensorHostObject::createSqueeze(jsi::Runtime& runtime) {
  auto squeezeFunc = [this](
                         jsi::Runtime& runtime,
                         const jsi::Value& thisValue,
                         const jsi::Value* arguments,
                         size_t count) -> jsi::Value {
    if (!((count == 1 && arguments[0].isNumber()) || count == 0)) {
      throw jsi::JSError(
          runtime, "Please enter an empty argument list or a single number.");
    }
    auto tensor = this->tensor;
    at::Tensor reshapedTensor;
    if (count == 0) {
      reshapedTensor = tensor.squeeze();
    } else if (count == 1) {
      int dim = arguments[0].asNumber();
      reshapedTensor = tensor.squeeze(dim);
    }

    auto tensorHostObject =
        std::make_shared<torchlive::torch::TensorHostObject>(
            runtime, reshapedTensor);
    return jsi::Object::createFromHostObject(runtime, tensorHostObject);
  };
  return jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, SQUEEZE), 1, squeezeFunc);
}

jsi::Function TensorHostObject::createSub(jsi::Runtime& runtime) {
  auto subFunc = [this](
                     jsi::Runtime& runtime,
                     const jsi::Value& thisValue,
                     const jsi::Value* arguments,
                     size_t count) {
    if (count < 1) {
      throw jsi::JSError(runtime, "At least 1 arg required");
    }
    auto alphaValue = utils::helpers::parseKeywordArgument(
        runtime, arguments, 1, count, "alpha");
    auto alphaScalar = alphaValue.isUndefined()
        ? at::Scalar(1)
        : at::Scalar(alphaValue.asNumber());
    auto tensor = this->tensor;
    if (arguments[0].isNumber()) {
      auto value = arguments[0].asNumber();
      tensor = tensor.sub(value, alphaScalar);
    } else {
      auto otherTensorHostObject =
          utils::helpers::parseTensor(runtime, &arguments[0]);
      auto otherTensor = otherTensorHostObject->tensor;
      tensor = tensor.sub(otherTensor, alphaScalar);
    }
    auto tensorHostObject =
        std::make_shared<torchlive::torch::TensorHostObject>(runtime, tensor);

    return jsi::Object::createFromHostObject(runtime, tensorHostObject);
  };

  return jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, SUB), 1, subFunc);
}

/**
 * Returns the k largest elements of the given input tensor along a given
 * dimension.
 *
 * https://pytorch.org/docs/stable/generated/torch.topk.html
 */
jsi::Function TensorHostObject::createTopK(jsi::Runtime& runtime) {
  auto topkFunc = [this](
                      jsi::Runtime& runtime,
                      const jsi::Value& thisValue,
                      const jsi::Value* arguments,
                      size_t count) {
    if (count < 1) {
      throw jsi::JSError(runtime, "This function requires at least 1 argument");
    }
    auto k = arguments[0].asNumber();
    auto resultTuple = this->tensor.topk(k);
    auto outputValuesTensorHostObject =
        std::make_shared<torchlive::torch::TensorHostObject>(
            runtime, std::get<0>(resultTuple));
    auto indicesInt64Tensor = std::get<1>(resultTuple);
    /**
     * NOTE: We need to convert the int64 type to int32 since Hermes does not
     * support Int64 data types yet.
     */
    auto outputIndicesTensorHostObject =
        std::make_shared<torchlive::torch::TensorHostObject>(
            runtime, indicesInt64Tensor.to(c10::ScalarType::Int));
    return jsi::Array::createWithElements(
        runtime,
        jsi::Object::createFromHostObject(
            runtime, outputValuesTensorHostObject),
        jsi::Object::createFromHostObject(
            runtime, outputIndicesTensorHostObject));
  };

  return jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, TOPK), 1, topkFunc);
}

jsi::Function TensorHostObject::createUnsqueeze(jsi::Runtime& runtime) {
  auto unsqueezeFunc = [this](
                           jsi::Runtime& runtime,
                           const jsi::Value& thisValue,
                           const jsi::Value* arguments,
                           size_t count) -> jsi::Value {
    auto tensor = this->tensor;
    int nDims = tensor.sizes().size();

    bool augumentCheck = (count == 1) && (arguments[0].isNumber()) &&
        ((int)arguments[0].asNumber() >= 0) &&
        ((int)arguments[0].asNumber() <= nDims);
    if (!augumentCheck) {
      throw jsi::JSError(
          runtime,
          "argument for squeeze() must be in range [0, " +
              std::to_string(nDims) + "]. " +
              std::to_string((int)arguments[0].asNumber()) + " is provided.\n");
    }

    at::Tensor reshapedTensor = tensor.unsqueeze(arguments[0].asNumber());

    auto tensorHostObject =
        std::make_shared<torchlive::torch::TensorHostObject>(
            runtime, reshapedTensor);
    return jsi::Object::createFromHostObject(runtime, tensorHostObject);
  };
  return jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, UNSQUEEZE), 1, unsqueezeFunc);
}

} // namespace torch
} // namespace torchlive