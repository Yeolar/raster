#include <stdlib.h>
#include <string.h>

namespace srilm {

/*
 * extend the size of an Array to accomodate size elements
 * Note we want to zero-initialize the data elements by default,
 * so we call their initializers with argument 0.  This means
 * that all data type used as arguments to the Array template
 * need to provide an initializer that accepts 0 as a single argument.
 */
template <class DataT>
void Array<DataT>::alloc(unsigned size, bool zero) {
  // size is highest index needed so size + 1 is number
  // of elements, and pad by half current size for growth.
  unsigned int newSize = size + 1 + alloc_size / 2;
  DataT *newData = new DataT[newSize];
  assert(newData != 0);

  if (zero) {
    memset(newData, 0, newSize * sizeof(DataT));
  }

  for (unsigned i = 0; i < alloc_size; i++) {
    newData[i] = _data[i];
  }

  delete[] _data;

  _data = newData;
  alloc_size = newSize;
}

template <class DataT>
Array<DataT> &Array<DataT>::operator=(const Array<DataT> &other) {
  if (&other == this) {
    return *this;
  }

  delete[] _data;

  _base = other._base;
  _size = other._size;

  // make new array only as large as needed
  alloc_size = other._size;

  _data = new DataT[alloc_size];
  assert(_data != 0);

  for (unsigned i = 0; i < _size; i++) {
    _data[i] = other._data[i];
  }

  return *this;
}

template <class DataT>
ZeroArray<DataT> &ZeroArray<DataT>::operator=(const ZeroArray<DataT> &other) {
  if (&other == this) {
    return *this;
  }

  delete[] Array<DataT>::_data;

  Array<DataT>::_base = other._base;
  Array<DataT>::_size = other._size;

  // make new array only as large as needed
  Array<DataT>::alloc_size = other._size;

  Array<DataT>::_data = new DataT[Array<DataT>::alloc_size];
  assert(Array<DataT>::_data != 0);

  for (unsigned i = 0; i < Array<DataT>::_size; i++) {
    Array<DataT>::_data[i] = other._data[i];
  }

  return *this;
}

}
