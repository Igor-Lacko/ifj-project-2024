import os
import subprocess

# Directory with test cases
# pre ostatnych: ~/ifj-project-2024/tests/test_files
test_dir = os.path.expanduser('../tests/test_files')

# Error code macros
SUCCESS = 0
LEXICAL_ERROR = 1
SYNTACTIC_ERROR = 2
SEMANTIC_ERROR_UNDEFINED = 3
SEMANTIC_ERROR_TYPECOUNT_FUNCTION = 4
SEMANTIC_ERROR_REDEFINED = 5
SEMANTIC_ERROR_MISSING_EXPR = 6
SEMANTIC_ERROR_TYPE_COMPATIBILITY = 7
SEMANTIC_ERROR_TYPE_DERIVATION = 8
SEMANTIC_ERROR_UNUSED_VARIABLE = 9
SEMANTIC_ERROR_OTHER = 10
INTERNAL_ERROR = 99

# Expected error codes for each test file
expected_errors = {
    '1lex_err_01.ifj24': LEXICAL_ERROR,
    '1lex_err_02.ifj24': LEXICAL_ERROR,
    '1lex_err_03.ifj24': LEXICAL_ERROR,
    '1lex_err_04.ifj24': LEXICAL_ERROR,
    '1lex_err_05.ifj24': LEXICAL_ERROR,
    '1lex_err_06.ifj24': LEXICAL_ERROR,
    '1lex_err_07.ifj24': LEXICAL_ERROR,
    '1lex_err_08.ifj24': SYNTACTIC_ERROR,
    '1lex_err_09.ifj24': SYNTACTIC_ERROR,
    '1lex_err_10.ifj24': LEXICAL_ERROR,
    '1lex_err_11.ifj24': LEXICAL_ERROR,
    '1lex_err_12.ifj24': LEXICAL_ERROR,
    '2synt_err_01.ifj24': SYNTACTIC_ERROR,
    '2synt_err_02.ifj24': SYNTACTIC_ERROR,
    '2synt_err_03.ifj24': SYNTACTIC_ERROR,
    '2synt_err_04.ifj24': SYNTACTIC_ERROR,
    '2synt_err_05.ifj24': SYNTACTIC_ERROR,
    '2synt_err_06.ifj24': SYNTACTIC_ERROR,
    '2synt_err_07.ifj24': SYNTACTIC_ERROR,
    '2synt_err_08.ifj24': SYNTACTIC_ERROR,
    '2synt_err_09.ifj24': SYNTACTIC_ERROR,
    '2synt_err_10.ifj24': SYNTACTIC_ERROR,
    '2synt_err_11.ifj24': SYNTACTIC_ERROR,
    '2synt_err_12.ifj24': SYNTACTIC_ERROR,
    '3sem_err_undefined_01.ifj24': SEMANTIC_ERROR_UNDEFINED,
    '3sem_err_undefined_02.ifj24': SEMANTIC_ERROR_UNDEFINED,
    '3sem_err_undefined_03.ifj24': SEMANTIC_ERROR_UNDEFINED,
    '3sem_err_undefined_04.ifj24': SEMANTIC_ERROR_UNDEFINED,
    '3sem_err_undefined_05.ifj24': SEMANTIC_ERROR_UNDEFINED,
    '3sem_err_undefined_06.ifj24': SEMANTIC_ERROR_UNDEFINED,
    '3sem_err_undefined_07.ifj24': SEMANTIC_ERROR_UNDEFINED,
    '3sem_err_undefined_08.ifj24': SEMANTIC_ERROR_UNDEFINED,
    '3sem_err_undefined_09.ifj24': SEMANTIC_ERROR_UNDEFINED,
    '3sem_err_undefined_10.ifj24': SEMANTIC_ERROR_UNDEFINED,
    '4sem_err_func_01.ifj24': SEMANTIC_ERROR_TYPECOUNT_FUNCTION,
    '4sem_err_func_02.ifj24': SEMANTIC_ERROR_TYPECOUNT_FUNCTION,
    '4sem_err_func_03.ifj24': SEMANTIC_ERROR_TYPECOUNT_FUNCTION,
    '4sem_err_func_04.ifj24': SEMANTIC_ERROR_TYPECOUNT_FUNCTION,
    '4sem_err_func_05.ifj24': SEMANTIC_ERROR_TYPECOUNT_FUNCTION,
    '4sem_err_func_06.ifj24': SEMANTIC_ERROR_TYPECOUNT_FUNCTION,
    '4sem_err_func_07.ifj24': SEMANTIC_ERROR_TYPECOUNT_FUNCTION,
    '4sem_err_func_08.ifj24': SEMANTIC_ERROR_TYPECOUNT_FUNCTION,
    '5sem_err_redef_01.ifj24': SEMANTIC_ERROR_REDEFINED,
    '5sem_err_redef_02.ifj24': SEMANTIC_ERROR_REDEFINED,
    '5sem_err_redef_03.ifj24': SEMANTIC_ERROR_REDEFINED,
    '5sem_err_redef_04.ifj24': SEMANTIC_ERROR_REDEFINED,
    '5sem_err_redef_05.ifj24': SEMANTIC_ERROR_REDEFINED,
    '5sem_err_redef_06.ifj24': SEMANTIC_ERROR_REDEFINED,
    '5sem_err_redef_07.ifj24': SEMANTIC_ERROR_REDEFINED,
    '5sem_err_redef_08.ifj24': SEMANTIC_ERROR_REDEFINED,
    '6sem_err_miss_expr_01.ifj24': SEMANTIC_ERROR_MISSING_EXPR,
    '6sem_err_miss_expr_02.ifj24': SEMANTIC_ERROR_MISSING_EXPR,
    '6sem_err_miss_expr_03.ifj24': SEMANTIC_ERROR_MISSING_EXPR,
    '6sem_err_miss_expr_04.ifj24': SEMANTIC_ERROR_MISSING_EXPR,
    '6sem_err_miss_expr_05.ifj24': SEMANTIC_ERROR_MISSING_EXPR,
    '6sem_err_miss_expr_06.ifj24': SEMANTIC_ERROR_MISSING_EXPR,
    '6sem_err_miss_expr_07.ifj24': SEMANTIC_ERROR_MISSING_EXPR,
    '6sem_err_miss_expr_08.ifj24': SEMANTIC_ERROR_MISSING_EXPR,
    '7sem_err_noncomp_01.ifj24': SEMANTIC_ERROR_TYPE_COMPATIBILITY,
    '7sem_err_noncomp_02.ifj24': SEMANTIC_ERROR_TYPE_COMPATIBILITY,
    '7sem_err_noncomp_03.ifj24': SEMANTIC_ERROR_TYPE_COMPATIBILITY,
    '7sem_err_noncomp_04.ifj24': SEMANTIC_ERROR_TYPE_COMPATIBILITY,
    '7sem_err_noncomp_05.ifj24': SEMANTIC_ERROR_TYPE_COMPATIBILITY,
    '7sem_err_noncomp_06.ifj24': SEMANTIC_ERROR_TYPE_COMPATIBILITY,
    '7sem_err_noncomp_07.ifj24': SEMANTIC_ERROR_TYPE_COMPATIBILITY,
    '7sem_err_noncomp_08.ifj24': SEMANTIC_ERROR_TYPE_COMPATIBILITY,
    '8sem_err_type_01.ifj24': SEMANTIC_ERROR_TYPE_DERIVATION,
    '8sem_err_type_02.ifj24': SEMANTIC_ERROR_TYPE_DERIVATION,
    '8sem_err_type_03.ifj24': SEMANTIC_ERROR_TYPE_DERIVATION,
    '8sem_err_type_04.ifj24': SEMANTIC_ERROR_TYPE_DERIVATION,
    '8sem_err_type_05.ifj24': SEMANTIC_ERROR_TYPE_DERIVATION,
    '8sem_err_type_06.ifj24': SEMANTIC_ERROR_TYPE_DERIVATION,
    '8sem_err_type_07.ifj24': SEMANTIC_ERROR_TYPE_DERIVATION,
    '8sem_err_type_08.ifj24': SEMANTIC_ERROR_TYPE_DERIVATION,
    '9sem_err_unused_variable_01.ifj24': SEMANTIC_ERROR_UNUSED_VARIABLE,
    '9sem_err_unused_variable_02.ifj24': SEMANTIC_ERROR_UNUSED_VARIABLE,
    '9sem_err_unused_variable_03.ifj24': SEMANTIC_ERROR_UNUSED_VARIABLE,
    '9sem_err_unused_variable_04.ifj24': SEMANTIC_ERROR_UNUSED_VARIABLE,
    '9sem_err_unused_variable_05.ifj24': SEMANTIC_ERROR_UNUSED_VARIABLE,
    '9sem_err_unused_variable_06.ifj24': SEMANTIC_ERROR_UNUSED_VARIABLE,
    '9sem_err_unused_variable_07.ifj24': SEMANTIC_ERROR_UNUSED_VARIABLE,
    '9sem_err_unused_variable_08.ifj24': SEMANTIC_ERROR_UNUSED_VARIABLE,
}

# Path to the compiler executable
# pre ostatnych co testuju: ~/ifj-project-2024/src/ifj24
compiler_path = './ifj24'

# Run each test and check output
for test_file, expected_code in expected_errors.items():
    file_path = os.path.join(test_dir, test_file)
    print(f"Running test: {test_file}")

    try:
        with open(file_path, 'r') as test_input:
            # Run the compiler with file input redirection
            result = subprocess.run([compiler_path], stdin=test_input, capture_output=True, text=True, timeout=5)
            actual_code = result.returncode
            # print(f"Output:\n{result.stdout}")
            # print(f"Error (if any):\n{result.stderr}")

            # Check if the result matches the expected error code
            if actual_code == expected_code:
                print(f"✅ Test passed for {test_file} (Expected: {expected_code}, Got: {actual_code})")
            else:
                print(f"❌ Test failed for {test_file} (Expected: {expected_code}, Got: {actual_code})")
                print(f"Output: {result.stderr.strip()}")
    except subprocess.TimeoutExpired:
        print(f"❌ Test timed out for {test_file}")
