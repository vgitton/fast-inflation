#pragma once

#include "../types.h"
#include "debug.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

/*! \defgroup serialization Serialization
 * \ingroup util
 *
 * \brief This module allows to read and write objects to disk in binary or text format
 *
 * The idea of this module is to define the util::FileStream class, and to define an abstract class util::Serializable
 * that can input or output to a util::FileStream.
 * Then, the util::FileStream can either be a util::InputFileStream or util::OutputFileStream, but the implementation of
 * the util::Serializable does not need to know whether it is reading or writing.
 * This prevents any mistake where one writes an object in one ordering of the attributes to the disk, but then reads it
 * in a different order.
 *
 * This mechanism is tested in a very simple context in the application user::file_stream.
 * */

namespace util {

class FileStream;

/*! \addtogroup serialization
 * @{
 * */

/*! \brief Abstract class that defines the interface of a class that is serializable.
 * \details Note that we choose an implementation that does not require
 * serializable objects to inherit from util::Serializable: as long
 * as they implement a suitable method io(util::FileStream& stream), they're
 * good to go. */
class Serializable {
  public:
    /*! \brief This method defines what data is to be read/written to disk, and in what order.
     *
     * Although the read/write mechanism is mostly meant to be implemented irrespectively of whether or not
     * the object is being read/written, the object to be seralized can still implement sanity checks of the
     * data being read by querying the util::FileStream::is_reading() method. */
    virtual void io(util::FileStream &stream) = 0;
};

/*! \brief Converts a numerical value into a string
 * \details We print numbers in hexadecimal to save a bit of space */
template <typename T>
typename std::enable_if<std::is_arithmetic<T>::value, std::string>::type
to_hex_str(T const &var) {
    std::stringstream string_stream;
    if (var < 0)
        string_stream << "-" << std::hex << (-var);
    else
        string_stream << "+" << std::hex << var;
    return string_stream.str();
}

/*! \brief Specialization to appropriately convert an ::Index into a string */
template <>
std::string to_hex_str(Index const &index);

/*! \brief Specialization to appropriately convert an inf::Outcome into a string */
template <>
std::string to_hex_str(uint8_t const &outcome);

/*! \brief Reads a numerical value from a hexadecimal string */
template <typename T>
typename std::enable_if<std::is_arithmetic<T>::value, void>::type
read_hex_str(T &var, std::string const &str) {
    // Need at least sign + one digit
    HARD_ASSERT_LTE(2, str.size())
    std::stringstream string_stream(str.substr(1));
    string_stream >> std::hex >> var;
    switch (str[0]) {
    case '+':
        break;
    case '-':
        var *= -1;
        break;
    default:
        THROW_ERROR(std::string("Unrecognized sign character ") + str[0])
    }
}

/*! \brief Specialization to appropriately read an ::Index from a string */
template <>
void read_hex_str(Index &var, std::string const &str);

/*! \brief Specialization to appropriately read an inf::Outcome from a string */
template <>
void read_hex_str(uint8_t &var, std::string const &str);

/*! \brief This abstract class provides a common interface to util::InputFileStream and util::OutputFileStream
 * to unify the read/write mechanism into a single mechanism. */
class FileStream {
  public:
    /*! \brief Whether the util::FileStream should read/write in binary or text format
     * \details Initially, it seemed that the binary formay would be more efficient, but actually,
     * for the inf::Outcome type, text of binary format takes up the same space.
     * On the other hand, for the ::Index type, the text format is typically more efficient, because
     * it will store small ::Index variables on only a few bytes. Thus, text is overall very nice,
     * and we made this the preferred format for our files. */
    enum class Format {
        binary, ///< Read/write data in binary format. The resulting files
                /// depend on the endianness to be interpreted correctly, so essentially, they cannot really
                /// be shared across different architectures.
        text,   ///< Read/write data in text format. This allows to inspect the files
                /// and to share them easily without formatting problems.
    };

    /*! \brief This version number should be changed if the class is modified: it is checked
     * when reading data to verify that we have not changed the read/write format without knowing. */
    static Index const version;

    /*! \brief A stream is either reading or writing, and is either in binary or text format */
    FileStream(bool reading, util::FileStream::Format format);
    virtual ~FileStream() {}

    /*! \brief This method can be queried by the util::Serializable classes to
     * implement sanity checks upon reading data. */
    bool is_reading() const;

    /*! \brief Read or write a string */
    void io(std::string &s);

    /*! \brief Read or write an arithmetic type */
    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type
    io(T &var) {
        switch (get_format()) {
        case util::FileStream::Format::binary: {
            io_bytes(reinterpret_cast<char *>(&var), sizeof(var));
            break;
        }
        case util::FileStream::Format::text: {
            std::string str;
            if (not is_reading())
                str = to_hex_str(var);

            io_str(str);

            if (is_reading())
                read_hex_str(var, str);

            break;
        }
        default:
            THROW_ERROR("switch")
        }
    }

    /*! \brief Read of write a util::Serializable object */
    template <typename T>
    // Use to have this, but more permissive to allow non-virtual classes to also
    // be serializable
    // typename std::enable_if<std::is_base_of<util::Serializable, T>::value, void>::type
    typename std::enable_if<!std::is_arithmetic<T>::value, void>::type
    io(T &obj) {
        obj.io(*this);
    }

    /*! \brief Read or write a `std::vector` */
    template <typename T>
    void io(std::vector<T> &vec) {
        Index size = vec.size();
        io(size);

        if (is_reading())
            vec.resize(size);

        for (T &element : vec)
            io(element);
    }

    /*! \brief This writes the specified \p value to the stream if the stream is writing,
     * and otherwise reads a value of type \p T from the stream and hard-asserts equality with \p value */
    template <typename T>
    void write_or_read_and_hard_assert(T const &value) {
        T value_bis;
        if (not is_reading())
            value_bis = value;
        io(value_bis);
        if (is_reading())
            HARD_ASSERT_EQUAL(value_bis, value)
    }

    /*! \brief Specialization to support literal strings */
    void write_or_read_and_hard_assert(char const *value);

  protected:
    /*! \brief Returns `m_extension`
     * \details This protected method and private `m_extension` ensures that child classes
     * cannot modify the extension. */
    std::string const &get_extension() const;
    /*! \brief Returns the read/write format in std format, based on `m_format` */
    std::ios_base::openmode get_std_openmode() const;
    /*! \brief Returns `m_format`
     * \details Since `m_format` is const, we could just make it protected, but this way is a bit more consistent
     * with the above protected getters */
    util::FileStream::Format get_format() const;

  private:
    /*! \brief Indicates whether the stream is in reading mode */
    bool const m_is_reading;
    /*! \brief The filename extension, currently `.binary` or `.txt` depending on `m_format` */
    std::string m_extension;
    /*! \brief Indicates whether the stream is in binary or text mode */
    util::FileStream::Format const m_format;

    // Virtual methods

    /*! \brief Virtual method that will be overridden depending on input or output
     * \details This method allows to read or write a count \p n of bytes starting at adress \p p. */
    virtual void io_bytes(char *p, Index n) = 0;
    /*! \brief Virtual method that will be overridden depending on input or output
     * \details This method allows to read or write a `std::string`. */
    virtual void io_str(std::string &s) = 0;
};

/*! \brief The specialization of util::FileStream to write to a file
 *
 * This class does not allow to output a const object that is non-numeric and non-string.
 * The reason is that we want a single-function I/O mechanism for serializable objects.
 * If writing const objects was crucial, we would need to duplicate the code in util::Serializable::io() to also
 * have a const version of the util::Serializable::io(), which defeats the purpose of this whole construction
 * (namely, avoiding separate input and output functions that may not agree with each other about the expected format).
 *
 * Note that in text mode, we do not allow to save strings that contain the newline character `\n`. This is asserted. */
class OutputFileStream : public util::FileStream {
  public:
    /*! \brief Initialize the stream to write in the specified \p format to \p filename, with \p metadata written
     * at the beginning of the file
     *
     * The \p metadata is written as a header to the file.
     * The idea is that this \p metadata will be checked when constructing a
     * corresponding util::InputFileStream. */
    OutputFileStream(std::string const &filename,
                     util::FileStream::Format format,
                     std::string const &metadata);
    ~OutputFileStream();

    // This is crucial to make the non-const versions visible
    using util::FileStream::io;

    /*! \brief This extra I/O method is there to avoid copying constant arguments. */
    void io(std::string const &s);

    /*! \brief This extra I/O method is there to avoid copying constant arguments. */
    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type
    io(T const &var) {
        switch (get_format()) {
        case util::FileStream::Format::binary: {
            io_bytes(reinterpret_cast<char const *>(&var), sizeof(var));
            break;
        }
        case util::FileStream::Format::text: {
            io_str(to_hex_str(var));
            break;
        }
        default:
            THROW_ERROR("switch")
        }
    }

  private:
    /*! \brief The output stream, appropriately set to binary or text mode */
    std::ofstream m_outstream;

    // Overrides (that call the const versions)

    void io_bytes(char *p, Index n) override;
    void io_bytes(char const *p, Index n);
    void io_str(std::string &s) override;
    void io_str(std::string const &s);
};

/*! \brief The specialization of util::FileStream to write to a file
 *
 * This is the class that should actually be instantiated in code to write
 * objects to disk. */
class InputFileStream : public util::FileStream {
  public:
    /*! \brief Initialize the stream to read from \p filename in the specified \p format, checking for \p expected_metadata
     * at the beginning of the file.
     *
     * The \p expected_metadata is expected to be written as a header to the file.
     * If it does not match the actual metadata, an error is thrown. */
    InputFileStream(std::string const &filename,
                    util::FileStream::Format format,
                    std::string const &expected_metadata);
    ~InputFileStream();

  private:
    /*! \brief The input stream, appropriately set to binary or text mode */
    std::ifstream m_instream;

    // Overrides

    void io_bytes(char *p, Index n) override;
    void io_str(std::string &s) override;
};

//! @}

} // namespace util
