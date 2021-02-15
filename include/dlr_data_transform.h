#ifndef DLR_DATA_TRANSFORM_H_
#define DLR_DATA_TRANSFORM_H_

#include <tvm/runtime/ndarray.h>

#include <nlohmann/json.hpp>

#include "dlr_common.h"

#define NUM_DATE_TIME_COLS 7

namespace dlr {

/*! \brief Base case for input transformers. */
class DLR_DLL Transformer {
 public:
  virtual void MapToNDArray(const nlohmann::json& input_json, const nlohmann::json& transform,
                            tvm::runtime::NDArray& input_array) const = 0;

  /*! \brief Helper function for TransformInput. Allocates NDArray to store mapped input data. */
  virtual tvm::runtime::NDArray InitNDArray(const nlohmann::json& input_json, DLDataType dtype,
                                            DLContext ctx) const;
};

class DLR_DLL FloatTransformer : public Transformer {
 private:
  /*! \brief When there is a value stof cannot convert to float, this value is used. */
  const float kBadValue = std::numeric_limits<float>::quiet_NaN();

 public:
  void MapToNDArray(const nlohmann::json& input_json, const nlohmann::json& transform,
                    tvm::runtime::NDArray& input_array) const;
};

class DLR_DLL CategoricalStringTransformer : public Transformer {
 private:
  /*! \brief When there is no mapping entry for TransformInput, this value is used. */
  const float kMissingValue = -1.0f;

 public:
  void MapToNDArray(const nlohmann::json& input_json, const nlohmann::json& transform,
                    tvm::runtime::NDArray& input_array) const;
};

class DLR_DLL DateTimeTransformer : public Transformer {
 private:
  /*! \brief When there is no mapping entry for TransformInput, this value is used. */
  const float kMissingValue = -1.0f;

  const std::map<std::string, int> month_to_digit = {
      {"Jan", 1}, {"Feb", 2}, {"Mar", 3}, {"Apr", 4},  {"May", 5},  {"Jun", 6},
      {"Jul", 7}, {"Aug", 8}, {"Sep", 9}, {"Oct", 10}, {"Nov", 11}, {"Dec", 12}};

  const std::map<uint8_t, uint8_t> num_days = {{1, 31}, {2, 28},  {3, 31},  {4, 30},
                                               {5, 31}, {6, 30},  {7, 31},  {8, 31},
                                               {9, 30}, {10, 31}, {11, 30}, {12, 31}};
  ;

  /*! \brief Split Strings in regard of given delimiter strings */
  std::string GetNextSplittedStr(std::string& input_string, std::string delimiter) const;

  /*! \brief Calculate if a given year is a leap year */
  bool IsLeapYear(int64_t year) const;

  /*! \brief Convert a given string to an array of digits representing [WEEKDAY,
   * YEAR, HOUR, MINUTE, SECOND, MONTH, WEEK_OF_YEAR*/
  void DigitizeDateTime(std::string& input_string, std::vector<int64_t>& datetime_digits) const;

  int64_t GetWeekDay(int64_t year, int64_t month, int64_t day) const;

  inline int64_t postive_modulo(int64_t i, int64_t n) const { return (i % n + n) % n; }

 public:
  void MapToNDArray(const nlohmann::json& input_json, const nlohmann::json& transform,
                    tvm::runtime::NDArray& input_array) const;

  tvm::runtime::NDArray InitNDArray(const nlohmann::json& input_json, DLDataType dtype,
                                    DLContext ctx) const;
};

/*! \brief Handles transformations of input and output data. */
class DLR_DLL DataTransform {
 private:
  /*! \brief When there is no mapping entry for TransformOutput, this value is used. */
  const char* kUnknownLabel = "<unseen_label>";

  /*! \brief Buffers to store transformed outputs. Maps output index to transformed data. */
  std::unordered_map<int, std::string> transformed_outputs_;

  /*! \brief Helper function for TransformInput. Interpets 1-D char input as JSON. */
  nlohmann::json GetAsJson(const int64_t* shape, const void* input, int dim) const;

  const std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Transformer>>>
  GetTransformerMap() const;

  template <typename T>
  nlohmann::json TransformOutputHelper1D(const nlohmann::json& mapping, const T* data,
                                         const std::vector<int64_t>& shape) const;

  template <typename T>
  nlohmann::json TransformOutputHelper2D(const nlohmann::json& mapping, const T* data,
                                         const std::vector<int64_t>& shape) const;

 public:
  /*! \brief Returns true if the input requires a data transform */
  bool HasInputTransform(const nlohmann::json& metadata) const;

  /*! \brief Returns true if the output requires a data transform */
  bool HasOutputTransform(const nlohmann::json& metadata, int index) const;

  /*! \brief Transform string input using CategoricalString input DataTransform. When
   * this map is present in the metadata file, the user is expected to provide string inputs to
   * SetDLRInput as 1-D vector. This function will interpret the user's input as JSON, apply the
   * mapping to convert strings to numbers, and produce a numeric NDArray which can be given to TVM
   * for the model input.
   */
  void TransformInput(const nlohmann::json& metadata, const int64_t* shape, const void* input,
                      int dim, const std::vector<DLDataType>& dtypes, DLContext ctx,
                      std::vector<tvm::runtime::NDArray>* tvm_inputs) const;

  /*! \brief Transform integer output using CategoricalString output DataTransform. When this map is
   * present in the metadata file, the model's output will be converted from an integer array to a
   * JSON string, where numbers are mapped back to strings according to the CategoricalString map in
   * the metadata file. A buffer is created to store the transformed output, and it's contents can
   * be accessed using the GetOutputShape, GetOutputSizeDim, GetOutput and GetOutputPtr methods.
   */
  void TransformOutput(const nlohmann::json& metadata, int index,
                       const tvm::runtime::NDArray& output_array);

  /*! \brief Get shape of transformed output. */
  void GetOutputShape(int index, int64_t* shape) const;

  /*! \brief Get size and dims of transformed output. */
  void GetOutputSizeDim(int index, int64_t* size, int* dim) const;

  /*! \brief Copy transformed output to a buffer. */
  void GetOutput(int index, void* output) const;

  /*! \brief Get pointer to transformed output data. */
  const void* GetOutputPtr(int index) const;
};

}  // namespace dlr

#endif  // DLR_DATA_TRANSFORM_H_
