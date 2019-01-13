/*!
 * Copyright (c) 2017-present, SeetaTech, Co.,Ltd.
 *
 * Licensed under the BSD 2-Clause License.
 * You should have received a copy of the BSD 2-Clause License
 * along with the software. If not, See,
 *
 *      <https://opensource.org/licenses/BSD-2-Clause>
 *
 * ------------------------------------------------------------
 */

#ifndef DRAGON_PYTHON_PY_TENSOR_H_
#define DRAGON_PYTHON_PY_TENOSR_H_

#include "py_dragon.h"

namespace dragon {

namespace python {

inline string ParseName(PyObject* self, PyObject* args) {
    char* cname;
    if (!PyArg_ParseTuple(args, "s", &cname))
        LOG(FATAL) << "Excepted a tensor name.";
    return cname;
}

inline PyObject* HasTensorCC(PyObject* self, PyObject* args) {
    if (ws()->HasTensor(ParseName(self, args))) Py_RETURN_TRUE;
    else Py_RETURN_FALSE;
}

inline PyObject* GetTensorNameCC(PyObject* self, PyObject* args) {
    string query = ws()->GetTensorName(ParseName(self, args));
    return String_AsPyUnicode(query);
}

inline PyObject* CreateTensorCC(PyObject* self, PyObject* args) {
    ws()->CreateTensor(ParseName(self, args));
    Py_RETURN_TRUE;
}

inline PyObject* CreateFillerCC(PyObject* self, PyObject* args) {
    PyObject* filler_string;
    if (!PyArg_ParseTuple(args, "S", &filler_string)) {
        PyErr_SetString(PyExc_ValueError,
            "Excepted a serialized string of TensorFiller.");
        return nullptr;
    }
    TensorFillerProto filler_proto;
    if (!filler_proto.ParseFromString(PyBytes_AsStringEx(filler_string))) {
        PyErr_SetString(PyExc_RuntimeError,
            "Failed to parse the TensorFiller.");
        return nullptr;
    }
    ws()->CreateFiller(filler_proto);
    ws()->CreateTensor(filler_proto.tensor());
    Py_RETURN_TRUE;
}

inline PyObject* GetFillerTypeCC(PyObject* self, PyObject* args) {
    const auto* f = ws()->GetFiller(ParseName(self, args));
    return String_AsPyUnicode(f->type());
}

inline PyObject* RenameTensorCC(PyObject* self, PyObject* args) {
    char* ori_name, *tar_name;
    if (!PyArg_ParseTuple(args, "ss", &ori_name, &tar_name)) {
        PyErr_SetString(PyExc_ValueError,
            "Excepted the original and target name.");
        return nullptr;
    }
    if (!ws()->HasTensor(tar_name)) {
        string err_msg = "Target name: " + string(tar_name)
           + " has not been registered in the current workspace.";
        PyErr_SetString(PyExc_ValueError, err_msg.c_str());
        return nullptr;
    }
    ws()->SetTensorProxy(ori_name, tar_name);
    Py_RETURN_TRUE;
}

PyObject* TensorFromShapeCC(PyObject* self, PyObject* args) {
    char* cname, *dtype;
    PyObject* shape, *device_option = nullptr;
    if (!PyArg_ParseTuple(args, "sOs|O", &cname, &shape, &dtype, &device_option)) {
        PyErr_SetString(PyExc_ValueError,
            "Excepted the name, shape, dtype and optional device option.");
        return nullptr;
    }
    const TypeMeta& meta = TypeStringToMeta(dtype);
    if (meta.id() == 0) {
        string err_msg = "Unsupported data type: " + string(dtype) + ".";
        PyErr_SetString(PyExc_TypeError, err_msg.c_str());
        return nullptr;
    }
    Tensor* tensor = ws()->CreateTensor(cname);
    if (meta.id() != tensor->meta().id() && tensor->meta().id() != 0)
        LOG(WARNING) << "Set Tensor(" << tensor->name() << ")"
        << " with different data type from original one.";
    int ndim = (int)PyList_Size(shape);
    CHECK_GT(ndim, 0)
        << "\nThe len of shape should be greater than 1. Got " << ndim << ".";
    vector<int64_t> dims;
    for (int i = 0; i < ndim; i++)
        dims.push_back((int64_t)_PyInt_AsInt(PyList_GetItem(shape, i)));
    tensor->Reshape(dims);
    DeviceOption dev_opt;
    if (device_option != nullptr) {
        if (!dev_opt.ParseFromString(PyBytes_AsStringEx(device_option))) {
            PyErr_SetString(PyExc_ValueError, "Failed to parse the DeviceOption.");
            return nullptr;
        }
    }
    if (dev_opt.device_type() == PROTO_CUDA) {
        CUDAContext ctx(dev_opt);
        ctx.SwitchToDevice();
        tensor->raw_mutable_data<CUDAContext>(meta);
    } else {
        tensor->raw_mutable_data<CPUContext>(meta);
    }
    Py_RETURN_TRUE;
}

PyObject* TensorFromPyArrayCC(PyObject* self, PyObject* args) {
    char* cname;
    PyArrayObject* original_array = nullptr;
    if (!PyArg_ParseTuple(args, "sO", &cname, &original_array)) {
        PyErr_SetString(PyExc_ValueError,
            "Failed to create tensor from numpy.ndarray.\n"
            "Excepted the name and numpy.ndarray both.");
        return nullptr;
    }
    PyArrayObject* array = PyArray_GETCONTIGUOUS(original_array);
    const TypeMeta& meta = TypeNPYToMeta(PyArray_TYPE(array));
    if (meta.id() == 0) {
        PyErr_SetString(PyExc_TypeError, "Unsupported data type.");
        return nullptr;
    }
    Tensor* tensor = ws()->CreateTensor(cname);
    tensor->SetMeta(meta);
    int ndim = PyArray_NDIM(array);
    npy_intp* npy_dims = PyArray_DIMS(array);
    vector<int64_t> dims;
    for (int i = 0; i < ndim; i++) dims.push_back(npy_dims[i]);
    tensor->Reshape(dims);
    auto* data = static_cast<void*>(PyArray_DATA(array));
    if (!tensor->has_memory()) {
        MixedMemory* memory(new MixedMemory());
        memory->set_cpu_data(data, tensor->nbytes());
        tensor->set_memory(memory);
    } else {
        if (tensor->DECREFPyArray) tensor->DECREFPyArray();
        tensor->memory()->set_cpu_data(data, tensor->nbytes()); 
    }
    //  follow the codes of PyTorch
    //  here we bind the DECREF to Tensor
    //  ResetTensor() or ResetWorkspace() can trigger it
    tensor->DECREFPyArray = [array]()->void { Py_XDECREF(array); };
    Py_RETURN_TRUE;
}

PyObject* TensorFromTensorCC(PyObject* self, PyObject* args) {
    char* dst_name, *src_name;
    PyObject* py_dst_ctx = nullptr, *py_src_ctx = nullptr;
    if (!PyArg_ParseTuple(args, "ssOO",
        &dst_name, &src_name, &py_dst_ctx, &py_src_ctx)) {
        PyErr_SetString(PyExc_ValueError,
            "Failed to create tensor from tensor.\n"
            "Excepted the (dest, src) name and context.");
        return nullptr;
    }
    DeviceOption dst_ctx, src_ctx;
    dst_ctx.ParseFromString(PyBytes_AsStringEx(py_dst_ctx));
    src_ctx.ParseFromString(PyBytes_AsStringEx(py_src_ctx));
    Tensor* srcT = ws()->GetTensor(src_name);
    Tensor* dstT = ws()->CreateTensor(dst_name);
    dstT->ReshapeLike(*srcT);
    const TypeMeta& meta = srcT->meta();
    if (dst_ctx.device_type() == PROTO_CUDA) {
        if (src_ctx.device_type() == PROTO_CUDA) {
            //  CUDA <- CUDA
            CUDAContext::Memcpy<CUDAContext, CUDAContext>(
                srcT->nbytes(),
                    dstT->raw_mutable_data<CUDAContext>(meta),
                        srcT->raw_data<CUDAContext>());
        } else {
            //  CUDA <- CPU
            CUDAContext::Memcpy<CUDAContext, CUDAContext>(
                srcT->nbytes(),
                    dstT->raw_mutable_data<CUDAContext>(meta),
                        srcT->raw_data<CPUContext>());
        }
    } else {
        if (src_ctx.device_type() == PROTO_CUDA) {
            //  CPU <- CUDA
            CUDAContext::Memcpy<CUDAContext, CUDAContext>(
                srcT->nbytes(),
                    dstT->raw_mutable_data<CPUContext>(meta),
                        srcT->raw_data<CUDAContext>());
        } else {
            //  CPU <- CPU
            CUDAContext::Memcpy<CUDAContext, CUDAContext>(
                srcT->nbytes(),
                    dstT->raw_mutable_data<CPUContext>(meta),
                        srcT->raw_data<CPUContext>());
        }
    }
    Py_RETURN_TRUE;
}

inline PyObject* TensorToPyArrayCC(PyObject* self, PyObject* args) {
    Tensor* tensor = ws()->GetTensor(ParseName(self, args));
    CHECK_GT(tensor->count(), 0);
    vector<npy_intp> dims;
    for (const auto dim : tensor->dims()) dims.push_back(dim);
    int npy_type = TypeMetaToNPY(tensor->meta());
    if (npy_type == -1) {
        string s = "Tensor(" + tensor->name() + ") with dtype." \
            + TypeMetaToString(tensor->meta()) + " is not supported by numpy.";
        PyErr_SetString(PyExc_RuntimeError, s.c_str());
        return nullptr;
    }
    auto* data = tensor->raw_mutable_data<CPUContext>();
    PyObject* array = PyArray_SimpleNewFromData(
        tensor->ndim(), dims.data(), npy_type, data);
    Py_XINCREF(array);
    return array;
}

inline PyObject* TensorToPyArrayExCC(PyObject* self, PyObject* args) {
    Tensor* tensor = ws()->GetTensor(ParseName(self, args));
    CHECK_GT(tensor->count(), 0);
    vector<npy_intp> dims;
    for (const auto dim : tensor->dims()) dims.push_back(dim);
    int npy_type = TypeMetaToNPY(tensor->meta());
    if (npy_type == -1) {
        string s = "Tensor(" + tensor->name() + ") with dtype." \
            + TypeMetaToString(tensor->meta()) + " is not supported by numpy.";
        PyErr_SetString(PyExc_RuntimeError, s.c_str());
        return nullptr;
    }
    auto* data = const_cast<void*>(tensor->raw_data<CPUContext>());
    PyObject* array = PyArray_SimpleNewFromData(
        tensor->ndim(), dims.data(), npy_type, data);
    Py_XINCREF(array);
    return array;
}

inline PyObject* ToCPUTensorCC(PyObject* self, PyObject* args) {
    string name = ParseName(self, args);
    Tensor* t = ws()->GetTensor(name);
    CHECK(t->has_memory()) << "\nTensor(" << name
        << ") does not initialize or had been reset.";
    t->memory()->ToCPU();
    Py_RETURN_TRUE;
}

inline PyObject* ToCUDATensorCC(PyObject* self, PyObject* args) {
#ifdef WITH_CUDA
    char* cname;
    int device_id;
    if (!PyArg_ParseTuple(args, "si", &cname, &device_id)) {
        PyErr_SetString(PyExc_ValueError,
            "Excepted the tensor name and device id.");
        return nullptr;
    }
    Tensor* t = ws()->GetTensor(cname);
    CHECK(t->has_memory()) << "\nTensor(" << cname
        << ") does not initialize or had been reset.";
    t->memory()->SwitchToCUDADevice(device_id);
#else
    CUDA_NOT_COMPILED;
#endif
    Py_RETURN_TRUE;
}

inline PyObject* GetTensorInfoCC(PyObject* self, PyObject* args) {
    /*!
     * Return shape will degrade performance remarkablely,
     * here we generalize info into 3 streams.
     * 
     * Stream #1: dtype, from_numpy, memory_info
     * Stream #2: shape
     * Stream #3: #1 + #2
     */
    char* cname; int stream;
    if (!PyArg_ParseTuple(args, "si", &cname, &stream))
        LOG(FATAL) << "Excepted a tensor name and a stream id.";
    Tensor* tensor = ws()->GetTensor(cname);
    PyObject* py_info = PyDict_New();
    if (stream != 2) {
        SetPyDictS2S(py_info, "dtype", TypeMetaToString(tensor->meta()).c_str());
        SetPyDictS2S(py_info, "from_numpy", ((tensor->DECREFPyArray) ? "1" : "0"));
        if (tensor->has_memory()) {
            Map<string, string> mem_info = tensor->memory()->info();
            for (auto& kv : mem_info)
                SetPyDictS2S(py_info, kv.first.c_str(), kv.second.c_str());
        }
    }
    if (stream >= 2) {
        PyObject* py_shape = PyTuple_New(tensor->ndim());
        for (int i = 0; i < tensor->ndim(); i++)
            PyTuple_SetItem(py_shape, i,
                PyInt_FromLong((long)tensor->dim(i)));
        PyDict_SetItemString(py_info, "shape", py_shape);
        Py_XDECREF(py_shape);
    }
    return py_info;
}

inline PyObject* ResetTensorCC(PyObject* self, PyObject* args) {
    ws()->ResetTensor(ParseName(self, args));
    Py_RETURN_TRUE;
}

inline PyObject* TensorsCC(PyObject* self, PyObject* args) {
    vector<string> tensors = ws()->GetTensors();
    PyObject* list = PyList_New(tensors.size());
    for (int i = 0; i < tensors.size(); i++)
        CHECK_EQ(PyList_SetItem(list, i, String_AsPyUnicode(tensors[i])), 0);
    return list;
}

}  // namespace python

}  // namespace dragon

#endif  // DRAGON_PYTHON_PY_TENSOR_H_