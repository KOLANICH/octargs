#include "test_fixture.hpp"

#include "../include/octargs/octargs.hpp"

namespace oct
{
namespace args
{

class storage_args_test : public test_fixture
{
private:
    void test_single_value()
    {
        argument_table args1("appname",
            {
                "--compress=zip",
                "--verbose",
                "--output=myfile.txt",
                "filename.txt",
            });

        struct settings
        {
            bool m_verbose;
            std::string m_compression_name;
            std::string m_output_file;
            std::vector<std::string> m_files;
        };

        storing_parser<settings> parser;

        parser.add_switch({ "--verbose" }).set_type_and_storage(&settings::m_verbose);
        parser.add_valued({ "--compress" }).set_type_and_storage(&settings::m_compression_name);
        parser.add_valued({ "--output" }).set_type_and_storage(&settings::m_output_file);
        parser.add_positional("FILES").set_type_and_storage(&settings::m_files);

        settings settings1;
        parser.parse(args1, settings1);

        CPPUNIT_ASSERT_EQUAL(true, settings1.m_verbose);
        CPPUNIT_ASSERT_EQUAL(std::string("zip"), settings1.m_compression_name);
        CPPUNIT_ASSERT_EQUAL(std::string("myfile.txt"), settings1.m_output_file);
        CPPUNIT_ASSERT_EQUAL(std::size_t(1), settings1.m_files.size());
        CPPUNIT_ASSERT_EQUAL(std::string("filename.txt"), settings1.m_files[0]);
    }

    void test_multi_value()
    {
        argument_table args1("appname",
            {
                "--compress=zip",
                "--include=file1.h",
                "--verbose",
                "--include=file2.h",
                "--include=file3.h",
                "filename1.txt",
                "filename2.txt",
            });

        struct settings
        {
            bool m_verbose;
            std::string m_compression_name;
            std::vector<std::string> m_include_files;
            std::vector<std::string> m_files;
        };

        storing_parser<settings> parser;

        parser.add_switch({ "--verbose" }).set_type_and_storage(&settings::m_verbose);
        parser.add_valued({ "--compress" }).set_type_and_storage(&settings::m_compression_name);
        parser.add_valued({ "--include" }).set_max_count_unlimited().set_type_and_storage(&settings::m_include_files);
        parser.add_positional("FILES").set_max_count_unlimited().set_type_and_storage(&settings::m_files);

        settings settings1;
        parser.parse(args1, settings1);

        CPPUNIT_ASSERT_EQUAL(true, settings1.m_verbose);
        CPPUNIT_ASSERT_EQUAL(std::string("zip"), settings1.m_compression_name);
        CPPUNIT_ASSERT_EQUAL(std::size_t(3), settings1.m_include_files.size());
        CPPUNIT_ASSERT_EQUAL(std::string("file1.h"), settings1.m_include_files[0]);
        CPPUNIT_ASSERT_EQUAL(std::string("file2.h"), settings1.m_include_files[1]);
        CPPUNIT_ASSERT_EQUAL(std::string("file3.h"), settings1.m_include_files[2]);
        CPPUNIT_ASSERT_EQUAL(std::size_t(2), settings1.m_files.size());
        CPPUNIT_ASSERT_EQUAL(std::string("filename1.txt"), settings1.m_files[0]);
        CPPUNIT_ASSERT_EQUAL(std::string("filename2.txt"), settings1.m_files[1]);
    }

    void test_multi_stored_in_single()
    {
        argument_table args1("appname", { "0", "1", "2" });

        struct settings
        {
            int m_value;
        };

        storing_parser<settings> parser;

        parser.add_positional("FILES").set_max_count_unlimited().set_type_and_storage(&settings::m_value);

        settings settings1;
        parser.parse(args1, settings1);

        CPPUNIT_ASSERT_EQUAL(int(2), settings1.m_value);
    }

    void test_types()
    {
        argument_table args1("appname",
            {
                "--int=5",
                "--int=0x14",
                "--double=-17.43",
                "--int=0765",
                "--longlong=12345678901",
            });

        struct settings
        {
            std::vector<int> m_ints;
            double m_double;
            long long m_longlong;
        };

        storing_parser<settings> parser;

        parser.add_valued({ "--int" }).set_max_count_unlimited().set_type_and_storage(&settings::m_ints);
        parser.add_valued({ "--double" }).set_type_and_storage(&settings::m_double);
        parser.add_valued({ "--longlong" }).set_type_and_storage(&settings::m_longlong);

        settings settings1;
        parser.parse(args1, settings1);

        CPPUNIT_ASSERT_EQUAL(std::size_t(3), settings1.m_ints.size());
        CPPUNIT_ASSERT_EQUAL(int(5), settings1.m_ints[0]);
        CPPUNIT_ASSERT_EQUAL(int(0x14), settings1.m_ints[1]);
        CPPUNIT_ASSERT_EQUAL(int(0765), settings1.m_ints[2]);
        CPPUNIT_ASSERT_EQUAL(double(-17.43), settings1.m_double);
        CPPUNIT_ASSERT_EQUAL((long long)(12345678901), settings1.m_longlong);
    }

    void test_null_convert_function()
    {
        argument_table args1("appname",
            {
                "--double=-17.43",
            });

        struct settings
        {
            double m_double;
        };

        storing_parser<settings> parser;
        auto& handler = parser.add_valued({ "--double" }).set_type<double>();

        settings settings1;
        parser.parse(args1, settings1);

        handler.set_convert_function(nullptr);
        CPPUNIT_ASSERT_THROW(parser.parse(args1, settings1), missing_converter);
    }

    void test_convert_function()
    {
        argument_table args1("appname",
            {
                "--double=-17.43",
            });

        struct settings
        {
            double m_double;
        };

        storing_parser<settings> parser;
        auto& handler
            = parser.add_valued({ "--double" })
                  .set_type<double>()
                  .set_convert_function([](const std::string& value_str) { return std::stod(value_str) + 10.0; })
                  .set_store_function([](settings& settings, double value) { settings.m_double = value; });

        settings settings1;
        parser.parse(args1, settings1);
        CPPUNIT_ASSERT_EQUAL(double(-7.43), settings1.m_double);

        handler.set_convert_function(
            [](const std::string & /*value_str*/) -> double { throw std::invalid_argument("value_str"); });

        CPPUNIT_ASSERT_THROW(parser.parse(args1, settings1), std::invalid_argument);
    }

    void test_check_function()
    {
        argument_table args1("appname",
            {
                "--double=-17.43",
            });
        argument_table args2("appname",
            {
                "--double=600",
            });

        struct settings
        {
            double m_double;
        };

        storing_parser<settings> parser;
        parser.add_valued({ "--double" })
            .set_type_and_storage<double>(&settings::m_double)
            .set_check_function([](double value) {
                if ((value < -100) || (value > 100))
                {
                    throw std::invalid_argument("bad value");
                }
            });

        settings settings1;

        parser.parse(args1, settings1);
        CPPUNIT_ASSERT_EQUAL(double(-17.43), settings1.m_double);

        CPPUNIT_ASSERT_THROW(parser.parse(args2, settings1), std::invalid_argument);
    }

    void test_store_function()
    {
        argument_table args1("appname",
            {
                "--double=-17.43",
            });

        struct settings
        {
        };
        double my_double;

        storing_parser<settings> parser;
        parser.add_valued({ "--double" }).set_type<double>().set_store_function([&my_double](double value) {
            my_double = value;
        });

        settings settings1;
        parser.parse(args1, settings1);
        CPPUNIT_ASSERT_EQUAL(double(-17.43), my_double);
    }

    template <typename parser_T>
    void check_parse_exception(parser_T& parser, const argument_table& args, parser_error_code code,
        const std::string& arg_name, const std::string& value_str)
    {
        try
        {
            parser.parse(args);
            CPPUNIT_ASSERT(false);
        }
        catch (const parser_error_ex<char>& exc)
        {
            CPPUNIT_ASSERT_EQUAL(exc.error_code(), code);
            CPPUNIT_ASSERT_EQUAL(exc.name(), arg_name);
            CPPUNIT_ASSERT_EQUAL(exc.value(), value_str);
        }
    }

    void test_bad_values()
    {
        struct settings
        {
            std::vector<int> m_ints;
            double m_double;
            long long m_longlong;
        };

        storing_parser<settings> parser;

        parser.add_valued({ "--int" }).set_max_count_unlimited().set_type_and_storage(&settings::m_ints);
        parser.add_valued({ "--double" }).set_type_and_storage(&settings::m_double);
        parser.add_valued({ "--longlong" }).set_type_and_storage(&settings::m_longlong);

        check_parse_exception(
            parser, argument_table("appname", { "--int=5.1" }), parser_error_code::CONVERSION_FAILED, "--int", "5.1");
        check_parse_exception(parser, argument_table("appname", { "--double=abc" }),
            parser_error_code::CONVERSION_FAILED, "--double", "abc");
        check_parse_exception(parser, argument_table("appname", { "--int=3", "--longlong", "1a" }),
            parser_error_code::CONVERSION_FAILED, "--longlong", "1a");

        check_parse_exception(parser, argument_table("appname", { "--double=3", "--double=5" }),
            parser_error_code::TOO_MANY_OCCURRENCES, "--double", "5");

        check_parse_exception(parser, argument_table("appname", { "--int=3", "7", "--longlong", "3" }),
            parser_error_code::SYNTAX_ERROR, "", "7");
        check_parse_exception(
            parser, argument_table("appname", { "--int=3", "aaa" }), parser_error_code::SYNTAX_ERROR, "", "aaa");

        check_parse_exception(
            parser, argument_table("appname", { "--double" }), parser_error_code::VALUE_MISSING, "--double", "");
        check_parse_exception(parser, argument_table("appname", { "--int=3", "--longlong" }),
            parser_error_code::VALUE_MISSING, "--longlong", "");

        parser.add_switch({ "--switch" });

        check_parse_exception(
            parser, argument_table("appname", { "--switch=3" }), parser_error_code::UNEXPECTED_VALUE, "--switch", "3");

        parser.add_valued({ "-s", "--string" }).set_min_count(1);

        check_parse_exception(
            parser, argument_table("appname", {}), parser_error_code::REQUIRED_ARGUMENT_MISSING, "-s", "");
    }

    void test_custom_type()
    {
        enum class format_code
        {
            UNKNOWN,
            HEX,
            DEC,
        };

        class format_code_converter
        {
        public:
            format_code operator()(const std::string& value_str) const
            {
                if (value_str == "hex")
                {
                    return format_code::HEX;
                }
                else if (value_str == "dec")
                {
                    return format_code::DEC;
                }
                throw conversion_error_ex<char>(value_str);
            }
        };

        struct settings
        {
            settings()
                : m_format(format_code::UNKNOWN)
                , m_multiformat()
            {
                // noop
            }

            format_code m_format;
            std::vector<format_code> m_multiformat;
        };

        storing_parser<settings> parser;

        parser.add_valued({ "--format" })
            .set_type<format_code, format_code_converter>()
            .set_storage(&settings::m_format);
        parser.add_valued({ "--multi" })
            .set_max_count_unlimited()
            .set_type<format_code, format_code_converter>()
            .set_storage(&settings::m_multiformat);

        settings settings1;
        auto results1 = parser.parse(argument_table("appname", { "--format=hex" }), settings1);
        CPPUNIT_ASSERT_EQUAL(std::size_t(1), results1.count("--format"));
        CPPUNIT_ASSERT_EQUAL(std::string("hex"), results1.values("--format")[0]);
        CPPUNIT_ASSERT_EQUAL(format_code::HEX, settings1.m_format);

        settings settings2;
        auto results2 = parser.parse(argument_table("appname", { "--format=dec" }), settings2);
        CPPUNIT_ASSERT_EQUAL(std::size_t(1), results2.count("--format"));
        CPPUNIT_ASSERT_EQUAL(std::string("dec"), results2.values("--format")[0]);
        CPPUNIT_ASSERT_EQUAL(format_code::DEC, settings2.m_format);

        CPPUNIT_ASSERT_THROW(parser.parse(argument_table("appname", { "--format=aaa" }), settings1), parser_error);

        settings settings3;
        auto results3
            = parser.parse(argument_table("appname", { "--multi=dec", "--multi", "hex", "--multi", "dec" }), settings3);
        CPPUNIT_ASSERT_EQUAL(std::size_t(0), results3.count("--format"));
        CPPUNIT_ASSERT_EQUAL(std::size_t(3), results3.count("--multi"));
        CPPUNIT_ASSERT_EQUAL(std::string("dec"), results3.values("--multi")[0]);
        CPPUNIT_ASSERT_EQUAL(std::string("hex"), results3.values("--multi")[1]);
        CPPUNIT_ASSERT_EQUAL(std::string("dec"), results3.values("--multi")[2]);
        CPPUNIT_ASSERT_EQUAL(std::size_t(3), settings3.m_multiformat.size());
        CPPUNIT_ASSERT_EQUAL(format_code::DEC, settings3.m_multiformat[0]);
        CPPUNIT_ASSERT_EQUAL(format_code::HEX, settings3.m_multiformat[1]);
        CPPUNIT_ASSERT_EQUAL(format_code::DEC, settings3.m_multiformat[2]);
    }

    CPPUNIT_TEST_SUITE(storage_args_test);
    CPPUNIT_TEST(test_single_value);
    CPPUNIT_TEST(test_multi_value);
    CPPUNIT_TEST(test_multi_stored_in_single);
    CPPUNIT_TEST(test_types);
    CPPUNIT_TEST(test_null_convert_function);
    CPPUNIT_TEST(test_convert_function);
    CPPUNIT_TEST(test_check_function);
    CPPUNIT_TEST(test_store_function);
    CPPUNIT_TEST(test_bad_values);
    CPPUNIT_TEST(test_custom_type);
    CPPUNIT_TEST_SUITE_END();
}; // namespace args

CPPUNIT_TEST_SUITE_REGISTRATION(storage_args_test);

} // namespace args
} // namespace oct
