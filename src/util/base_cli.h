#pragma once

#include <map>
#include <string>
#include <vector>

namespace util {

/*! \ingroup misc
 *
 * \brief This class provides support to extract command-line options of the form `--option value`
 *
 * A concrete program with command-line arguements can use this class as a backend to automate the process of extracting
 * named optional parameters, taking care of necessary string-to-other-type conversions. */
class BaseCLI {
  public:
    /*! \brief The constructor extracts the parameters into named and unnamed parameters.
     *
     * The parameters can then be extracted and converted to the desired type using the extraction methods.
     * The parameters \p argc and \p argv should be those of the `main(int argc, char** argv)` function. */
    BaseCLI(int argc, char **argv);
    //! \cond
    BaseCLI(BaseCLI const &) = delete;
    BaseCLI(BaseCLI &&) = delete;
    BaseCLI &operator=(BaseCLI const &) = delete;
    BaseCLI &operator=(BaseCLI &&) = delete;
    //! \endcond

    /*! \brief Extracts a named optional string parameter
     * \return The corresponding string value for the parameter if it was found in the command-line parameters,
     * or \p default_value else. */
    std::string extract_str_option(std::string const &name, std::string const &default_value);

    /*! \brief This method extracts and remove the requested option `--name`, and tries to obtain
     * an unsigned int from it.
     * \return The corresponding unsigned int value for the parameter if it was found in the command-line parameters,
     * or \p default_value else. */
    unsigned int extract_uint_option(std::string const &name, unsigned int default_value);

    /*! \brief This method extracts and remove the first unnamed argument.
     * \return The corresponding string value if it was found in the command-line parameters,
     * or \p default_value else. */
    std::string extract_str_arg(std::string const &default_value);

    /*! \brief This method does nothing if all options have been extracted by the above methods,
     * otherwise, it displays the unrecognized options and throws an error. */
    void hard_assert_empty() const;

  private:
    /*! \brief The list of named parameters of the form `--key value` found in the `argv` passed to the constructor.
     *
     * This is a vector, not a map, to allow to store duplicate options, e.g., when calling `./debug_inf --verb 2 --verb 3` */
    std::vector<std::pair<std::string, std::string>> m_named_args;
    /*! \brief The list of unnamed parameters found in the `argv` passed to the constructor. */
    std::vector<std::string> m_unnamed_args;
};

} // namespace util
