#include "file_stream.h"

#include "debug.h"
#include "logger.h"

// For std::as_const
#include <utility>

template <>
std::string util::to_hex_str(Index const &index) {
    std::stringstream string_stream;
    string_stream << std::hex << index;
    return string_stream.str();
}

template <>
std::string util::to_hex_str(uint8_t const &outcome) {
    std::stringstream string_stream;
    string_stream << std::hex << static_cast<int>(outcome);
    return string_stream.str();
}

template <>
void util::read_hex_str(Index &index, std::string const &str) {
    std::stringstream string_stream(str);
    string_stream >> std::hex >> index;
}

/*! \brief Specialization to appropriately read an inf::Outcome from a string */
template <>
void util::read_hex_str(uint8_t &outcome, std::string const &str) {
    std::stringstream string_stream(str);
    int outcome_as_int;
    string_stream >> std::hex >> outcome_as_int;
    outcome = static_cast<uint8_t>(outcome_as_int);
}

// util::FileStream

Index const util::FileStream::version = 5;

util::FileStream::FileStream(bool is_reading, util::FileStream::Format format)
    : m_is_reading(is_reading),
      m_extension(""),
      m_format(format) {

    switch (format) {
    case util::FileStream::Format::binary:
        m_extension = ".binary";
        break;
    case util::FileStream::Format::text:
        m_extension = ".txt";
        break;
    default:
        THROW_ERROR("switch")
    }
}

bool util::FileStream::is_reading() const {
    return m_is_reading;
}

void util::FileStream::io(std::string &s) {
    io_str(s);
}

void util::FileStream::write_or_read_and_hard_assert(char const *value) {
    write_or_read_and_hard_assert(std::string(value));
}

std::string const &util::FileStream::get_extension() const {
    return m_extension;
}

std::ios_base::openmode util::FileStream::get_std_openmode() const {
    std::ios_base::openmode open_mode;

    if (m_is_reading)
        open_mode = std::ios_base::in;
    else
        open_mode = std::ios_base::out;

    if (m_format == util::FileStream::Format::binary)
        open_mode |= std::ios_base::binary;

    return open_mode;
}

util::FileStream::Format util::FileStream::get_format() const {
    return m_format;
}

// util::OutputFileStream

util::OutputFileStream::OutputFileStream(std::string const &filename, util::FileStream::Format format, std::string const &metadata)
    : util::FileStream(false, format),
      m_outstream(filename + get_extension(), get_std_openmode()) {

    if (not m_outstream.is_open())
        THROW_ERROR("Could not open the file " + filename + get_extension())

    io(util::FileStream::version);
    io(metadata);

    util::logger << "Writing to file " << filename << get_extension()
                 << "." << util::cr << "Metadata: "
                 << metadata << util::cr;
}

util::OutputFileStream::~OutputFileStream() {
    m_outstream.close();
}

void util::OutputFileStream::io(std::string const &s) {
    io_str(s);
}

void util::OutputFileStream::io_bytes(char *p, Index n) {
    io_bytes(const_cast<char const *>(p), n);
}

void util::OutputFileStream::io_bytes(char const *p, Index n) {
    m_outstream.write(p, n);
}

void util::OutputFileStream::io_str(std::string &s) {
    io_str(const_cast<std::string const &>(s));
}

void util::OutputFileStream::io_str(std::string const &s) {
    switch (get_format()) {
    case util::FileStream::Format::binary:
        io(s.size());
        io_bytes(s.data(), s.size());
        break;
    case util::FileStream::Format::text:
        // We can't output strings that contains newline, because we use this as a separator
        // in text files. (In binary format this restriction is lifted though.)
        for (char const c : s)
            HARD_ASSERT_TRUE(c != '\n')
        m_outstream << s << "\n";
        break;
    default:
        THROW_ERROR("switch")
    }
}

// util::InputFileStream

util::InputFileStream::InputFileStream(std::string const &filename,
                                       util::FileStream::Format format,
                                       std::string const &expected_metadata)
    : util::FileStream(true, format),
      m_instream(filename + get_extension(), get_std_openmode()) {
    if (not m_instream.is_open())
        THROW_ERROR("Could not open the file " + filename + get_extension())

    Index version_read;
    io(version_read);
    HARD_ASSERT_EQUAL(version_read, util::FileStream::version)

    std::string metadata_read;
    io(metadata_read);
    HARD_ASSERT_EQUAL(metadata_read, expected_metadata)

    util::logger << "Reading file " << filename << get_extension()
                 << "." << util::cr << "Metadata: "
                 << metadata_read << util::cr;
}

util::InputFileStream::~InputFileStream() {
    m_instream.close();
}

void util::InputFileStream::io_bytes(char *p, Index n) {
    ASSERT_TRUE(p != nullptr)
    m_instream.read(p, n);
    HARD_ASSERT_EQUAL(m_instream.gcount(), static_cast<long int>(n))
}

void util::InputFileStream::io_str(std::string &s) {
    switch (get_format()) {
    case util::FileStream::Format::binary:
        Index size;
        io(size);
        s = std::string(size, '#');
        io_bytes(s.data(), size);
        break;
    case util::FileStream::Format::text:
        HARD_ASSERT_TRUE(std::getline(m_instream, s, '\n'));
        break;
    default:
        THROW_ERROR("switch")
    }
}
