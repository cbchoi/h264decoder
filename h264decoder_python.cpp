#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <cassert>

// python string api, see
// https://docs.python.org/2/c-api/string.html
extern "C" {
  #include <Python.h>
}

#include <boost/python.hpp>
#include <boost/python/str.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/module.hpp>
#include <boost/python/class.hpp>
namespace py = boost::python;

#include "h264decoder.hpp"

using ubyte = unsigned char;


#include <iostream>
#include <fstream>
#include <sstream>
int global_idx = 0;

class GILScopedReverseLock
{
  // see https://docs.python.org/2/c-api/init.html (Releasing the GIL ...)  
public:
  GILScopedReverseLock() 
    : state(nullptr)
  {
    unlock();
  }
  
  ~GILScopedReverseLock()
  {
    lock();
  }
  
  void lock() 
  {
    // Allow successive calls to lock. 
    // E.g. lock() followed by destructor.
    if (state != nullptr)
    {
      PyEval_RestoreThread(state);
      state = nullptr;
    }
  }
  
  void unlock()
  {
    assert (state == nullptr);
    state = PyEval_SaveThread();
  }
  
  GILScopedReverseLock(const GILScopedReverseLock &) = delete;
  GILScopedReverseLock(const GILScopedReverseLock &&) = delete;
  GILScopedReverseLock operator=(const GILScopedReverseLock &) = delete;
  GILScopedReverseLock operator=(const GILScopedReverseLock &&) = delete;
private:
  PyThreadState *state;
};


/* The class wrapped in python via boost::python */
class PyH264Decoder
{
  H264Decoder decoder;
  ConverterRGB24 converter;

  /* Extract frames from input stream. Stops at frame boundaries and returns the number of consumed bytes
   * in num_consumed.
   * 
   * If a frame is completed, is_frame_available is set to true, and the returned python tuple contains
   * formation about the frame as well as the frame buffer memory. 
   * 
   * Else, i.e. all data in the buffer is consumed, is_frame_available is set to false. The returned tuple
   * contains dummy data.
   */ 
  py::tuple decode_frame_impl(const ubyte *data, ssize_t num, ssize_t &num_consumed, bool &is_frame_available);
  
public:
  /* Decoding style analogous to c/c++ way. Stop at frame boundaries. 
   * Return tuple containing frame data as above as nested tuple, and an integer telling how many bytes were consumed.  */
  //py::tuple decode_frame(const py::str &data_in_str);
  /* Process all the input data and return a list of all contained frames. */
  py::list  decode(const py::object& data_in_str);
};


py::tuple PyH264Decoder::decode_frame_impl(const ubyte *data_in, ssize_t len, ssize_t &num_consumed, bool &is_frame_available)
{
  GILScopedReverseLock gilguard;
  //std::cout << len << std::endl;
  num_consumed = decoder.parse((ubyte*)data_in, len);
  
  if (is_frame_available = decoder.is_frame_available())
  {
    const auto &frame = decoder.decode_frame();
    int w, h; std::tie(w,h) = width_height(frame);
    Py_ssize_t out_size = converter.predict_size(w,h);

    gilguard.lock();
    //   Construction of py::handle causes ... TODO: WHAT? No increase of ref count ?!
    //py::object py_out_str(py::handle<>(py::borrowed(PyBytes_FromStringAndSize(NULL, out_size))));
    py::object py_out_str(py::handle<>(py::borrowed(PyBytes_FromStringAndSize(NULL, out_size))));
    Py_buffer pybuf;
    
    if(PyObject_GetBuffer(py_out_str.ptr(), &pybuf, PyBUF_SIMPLE) != -1)
    {
      void* buf = pybuf.buf;
      const auto &rgbframe = converter.convert(frame, (ubyte*)buf);
      return py::make_tuple(py_out_str, w, h, row_size(rgbframe));
    }
    else
      return py::make_tuple(py::object(), 0, 0, 0);
    /*gilguard.unlock();
    
    
    gilguard.lock();
    std::cout << "available" << std::endl;
    return py::make_tuple(py_out_str, w, h, 0);
    //return py::make_tuple(py::object(), 0, 0, 0);*/
  }
  else
  {
    gilguard.lock();
    return py::make_tuple(py::object(), 0, 0, 0);
  }
}

/*
py::tuple PyH264Decoder::decode_frame(const py::str &data_in_str)
{
  ssize_t len = PyMapping_Size(data_in_str.ptr());
  const ubyte* data_in = (const ubyte*)(data_in_str.ptr());

  ssize_t num_consumed = 0;
  bool is_frame_available = false;
  auto frame = decode_frame_impl(data_in, len, num_consumed, is_frame_available);
  
  return py::make_tuple(frame, num_consumed);
}
*/

py::list PyH264Decoder::decode(const py::object& data_in_str)
{
  py::stl_input_iterator<unsigned char> begin(data_in_str), end;
  
  // Copy the py_buffer into a local buffer with known continguous memory.
  //std::stringstream sstream;
  //sstream << "./test_" << global_idx++ << ".bin";

  std::vector<ubyte> buffer(begin, end);
  ssize_t len = buffer.size();

  const ubyte* data_in = (const ubyte*) reinterpret_cast<unsigned char*>(&buffer[0]);
  
  //std::ofstream fout(sstream.str().c_str());
  //fout.write((char*)data_in, len);
  //fout.close();

  //ssize_t len = data_in_str.len;
  //const ubyte* data_in = (const ubyte*)(data_in_str.buf);
  //ssize_t len = PyMapping_Size(data_in_str.ptr());
  //const ubyte* data_in = (const ubyte*)(data_in_str.ptr());
  
  py::list out;
  
  try
  {
    while (len > 0)
    {
      ssize_t num_consumed = 0;
      bool is_frame_available = false;
      
      try
      {
        py::tuple frame = decode_frame_impl(data_in, len, num_consumed, is_frame_available);
        if (is_frame_available)
        {
          out.append(frame);
        }
      }
      catch (const H264DecodeFailure &e)
      {
        if (num_consumed <= 0)
          // This case is fatal because we cannot continue to move ahead in the stream.
          throw e;
      }
      
      len -= num_consumed;
      data_in += num_consumed;
    }
  }
  catch (const H264DecodeFailure &e)
  {
  }
  return out;
}


BOOST_PYTHON_MODULE(libh264decoder)
{
  PyEval_InitThreads(); // need for release of the GIL (http://stackoverflow.com/questions/8009613/boost-python-not-supporting-parallelism)
  py::class_<PyH264Decoder>("H264Decoder")
                            //.def("decode_frame", &PyH264Decoder::decode_frame)
                            .def("decode", &PyH264Decoder::decode);
  py::def("disable_logging", disable_logging);
}