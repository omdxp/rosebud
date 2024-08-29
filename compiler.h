#ifndef ROSEBUDCOMPILER_H
#define ROSEBUDCOMPILER_H
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define S_EQ(str1, str2) (str1 && str2 && (strcmp(str1, str2) == 0))
#define STACK_PUSH_SIZE 4
#define C_STACK_ALIGNMENT 16
#define C_ALIGN(size)                                                          \
  (size % C_STACK_ALIGNMENT)                                                   \
      ? size + (C_STACK_ALIGNMENT - (size % C_STACK_ALIGNMENT))                \
      : size

struct pos {
  int line;
  int col;
  const char *filename;
};

#define NUMERIC_CASE                                                           \
  case '0':                                                                    \
  case '1':                                                                    \
  case '2':                                                                    \
  case '3':                                                                    \
  case '4':                                                                    \
  case '5':                                                                    \
  case '6':                                                                    \
  case '7':                                                                    \
  case '8':                                                                    \
  case '9'

#define OPERATOR_CASE_EXCLUDING_DIVISION                                       \
  case '+':                                                                    \
  case '-':                                                                    \
  case '*':                                                                    \
  case '>':                                                                    \
  case '<':                                                                    \
  case '^':                                                                    \
  case '%':                                                                    \
  case '!':                                                                    \
  case '=':                                                                    \
  case '~':                                                                    \
  case '|':                                                                    \
  case '&':                                                                    \
  case '(':                                                                    \
  case '[':                                                                    \
  case ',':                                                                    \
  case '.':                                                                    \
  case '?'

#define SYMBOL_CASE                                                            \
  case '{':                                                                    \
  case '}':                                                                    \
  case ':':                                                                    \
  case ';':                                                                    \
  case '#':                                                                    \
  case '\\':                                                                   \
  case ')':                                                                    \
  case ']'

enum { LEXICAL_ANALYSIS_ALL_OK, LEXICAL_ANALYSIS_INPUT_ERROR };

enum {
  TOKEN_TYPE_IDENTIFIER,
  TOKEN_TYPE_KEYWORD,
  TOKEN_TYPE_OPERATOR,
  TOKEN_TYPE_SYMBOL,
  TOKEN_TYPE_NUMBER,
  TOKEN_TYPE_STRING,
  TOKEN_TYPE_COMMENT,
  TOKEN_TYPE_NEWLINE,
};

enum {
  NUMBER_TYPE_NORMAL,
  NUMBER_TYPE_LONG,
  NUMBER_TYPE_FLOAT,
  NUMBER_TYPE_DOUBLE,
};

struct token {
  int type;
  int flags;
  struct pos pos;
  union {
    char cval;
    const char *sval;
    unsigned int inum;
    unsigned long lnum;
    unsigned long long llnum;
    void *any;
  };

  struct token_number {
    int type;
  } num;

  // True if there is a whitespace between the current token and next token
  bool whitespace;

  // (5+10+20)
  const char *between_brackets;
};

struct lex_process;
typedef char (*LEX_PROCESS_NEXT_CHAR)(struct lex_process *process);
typedef char (*LEX_PROCESS_PEEK_CHAR)(struct lex_process *process);
typedef void (*LEX_PROCESS_PUSH_CHAR)(struct lex_process *process, char c);
struct lex_process_functions {
  LEX_PROCESS_NEXT_CHAR next_char;
  LEX_PROCESS_PEEK_CHAR peek_char;
  LEX_PROCESS_PUSH_CHAR push_char;
};

struct lex_process {
  struct pos pos;
  struct vector *token_vec;
  struct compile_process *compiler;

  /**
   * ((50))
   */
  int current_expression_count;
  struct buffer *parenthesis_buffer;
  struct lex_process_functions *function;

  // This will be private data that the lexer does not understand
  // but the person using the lexer does understand.
  void *private;
};

enum {
  COMPILER_FILE_COMPILED_OK,
  COMPILER_FAILED_WITH_ERRORS,
};

struct scope {
  int flags;

  // void*
  struct vector *entities;

  // total number of bytes this scope occupies. Aligned to 16 bytes.
  size_t size;

  // null if this is the global scope
  struct scope *parent;
};

struct codegen_entry_point {
  // ID of the entry point
  int id;
};

struct codegen_exit_point {
  // ID of the exit point
  int id;
};

struct string_table_element {
  // string that the element is related to
  const char *str;

  // assembly label for the string in the .rodata section
  const char label[50];
};

struct code_generator {
  // vector of struct string_table_element*
  struct vector *string_table;

  // vector of struct codegen_entry_point*
  struct vector *entry_points;

  // vector of struct codegen_exit_point*
  struct vector *exit_points;
};

enum {
  SYMBOL_TYPE_NODE,
  SYMBOL_TYPE_NATIVE_FUNCTION,
  SYMBOL_TYPE_UNKNOWN,
};

struct symbol {
  const char *name;
  int type;
  void *data;
};

struct resolver_process;

struct compile_process {
  // The flags in regard on how this file should be compiled
  int flags;

  struct pos pos;
  struct compile_process_input_file {
    FILE *fp;
    const char *abs_path;
  } cfile;

  // The vector of tokens from lexical analysis
  struct vector *token_vec;

  struct vector *node_vec;
  struct vector *node_tree_vec;
  FILE *ofile;

  struct {
    struct scope *root;
    struct scope *current;
  } scope;

  struct {
    // current active symbol table
    struct vector *table;

    // all symbol tables
    struct vector *tables;
  } symbols;

  // pointer to code generator
  struct code_generator *generator;

  // pointer to resolver process
  struct resolver_process *resolver;
};

enum { PARSE_ALL_OK, PARSE_GENERAL_ERROR };

enum { CODEGEN_ALL_OK, CODEGEN_GENERAL_ERROR };

enum {
  NODE_TYPE_EXPRESSION,
  NODE_TYPE_EXPRESSION_PARENTHESIS,
  NODE_TYPE_NUMBER,
  NODE_TYPE_IDENTIFIER,
  NODE_TYPE_STRING,
  NODE_TYPE_VARIABLE,
  NODE_TYPE_VARIABLE_LIST,
  NODE_TYPE_FUNCTION,
  NODE_TYPE_BODY,
  NODE_TYPE_STATEMENT_RETURN,
  NODE_TYPE_STATEMENT_IF,
  NODE_TYPE_STATEMENT_ELSE,
  NODE_TYPE_STATEMENT_WHILE,
  NODE_TYPE_STATEMENT_DO_WHILE,
  NODE_TYPE_STATEMENT_FOR,
  NODE_TYPE_STATEMENT_BREAK,
  NODE_TYPE_STATEMENT_CONTINUE,
  NODE_TYPE_STATEMENT_SWITCH,
  NODE_TYPE_STATEMENT_CASE,
  NODE_TYPE_STATEMENT_DEFAULT,
  NODE_TYPE_STATEMENT_GOTO,

  NODE_TYPE_UNARY,
  NODE_TYPE_TENARY,
  NODE_TYPE_LABEL,
  NODE_TYPE_STRUCT,
  NODE_TYPE_UNION,
  NODE_TYPE_BRACKET,
  NODE_TYPE_CAST,
  NODE_TYPE_BLANK
};

enum {
  NODE_FLAG_INSIDE_EXPRESSION = 0b00000001,
  NODE_FLAG_IS_FORWARD_DECLARATION = 0b00000010,
  NODE_FLAG_HAS_VARIABLE_COMBINED = 0b00000100,
};

struct array_brackets {
  // vector of struct node*
  struct vector *n_brackets;
};

struct node;
struct datatype {
  int flags;
  // .i.e int, char, float, double, etc
  int type;

  // .e.g long int a; int is the secondary datatype
  struct datatype *secondary;

  // .i.e int, char, float, double, etc
  const char *type_str;

  // size of the datatype
  size_t size;

  // .e.g int **pointer = 0;
  int pointer_depth;

  union {
    struct node *struct_node;
    struct node *union_node;
  };

  struct array {
    struct array_brackets *brackets;
    // size of the array in bytes (DATATYPE_SIZE * array.size)
    size_t size;
  } array;
};

struct stack_frame_data {
  // data type that the stack frame is for
  struct datatype type;
};

struct stack_frame_element {
  // stack frame flags
  int flags;

  // type of the stack frame element
  int type;

  // name of the stack frame element
  const char *name;

  // offset from the base pointer
  int offset_from_bp;

  // data for the stack frame element
  struct stack_frame_data data;
};

#define STACK_PUSH_SIZE 4

enum {
  STACK_FRAME_ELEMENT_TYPE_LOCAL_VARIABLE,
  STACK_FRAME_ELEMENT_TYPE_SAVED_REGISTER,
  STACK_FRAME_ELEMENT_TYPE_SAVED_BASE_POINTER,
  STACK_FRAME_ELEMENT_TYPE_PUSHED_VALUE,
  STACK_FRAME_ELEMENT_TYPE_UNKNOWN,
};

enum {
  STACK_FRAME_ELEMENT_FLAG_IS_PUSHED_ADDRESS = 0b00000001,
  STACK_FRAME_ELEMENT_FLAG_ELEMENT_NOT_FOUND = 0b00000010,
  STACK_FRAME_ELEMENT_FLAG_IS_NUMERICAL = 0b00000100,
  STACK_FRAME_ELEMENT_FLAG_HAS_DATA_TYPE = 0b00001000,
};

void stackframe_pop(struct node *func_node);
struct stack_frame_element *stackframe_back(struct node *func_node);
struct stack_frame_element *stackframe_back_expect(struct node *func_node,
                                                   int expecting_type,
                                                   const char *expecting_name);
void stackframe_pop_expecting(struct node *func_node, int expecting_type,
                              const char *expecting_name);
void stackframe_peek_start(struct node *func_node);
struct stack_frame_element *stackframe_peek(struct node *func_node);
void stackframe_push(struct node *func_node,
                     struct stack_frame_element *element);
void stackframe_sub(struct node *func_node, int type, const char *name,
                    size_t amount);
void stackframe_add(struct node *func_node, int type, const char *name,
                    size_t amount);
void stackframe_assert_empty(struct node *func_node);

struct parsed_switch_case {
  // index of parsed case
  int index;
};

struct node;
struct unary {
  // operator of the unary expression (i.e. !, ~, *, etc), even for multiple
  // pointer access only one operator is stored
  const char *op;

  // operand of the unary expression
  struct node *operand;

  union {
    struct indirection {
      // depth of the indirection
      int depth;
    } indirection;
  };
};

struct node {
  int type;
  int flags;

  struct pos pos;

  struct node_binded {
    // Pointer to body node
    struct node *owner;

    // Pointer to function this node is in
    struct node *function;
  } binded;

  union {
    struct exp {
      struct node *left;
      struct node *right;
      const char *op;
    } exp;

    struct parenthesis {
      // expression inside the parenthesis node
      struct node *exp;
    } paren;

    struct var {
      struct datatype type;
      int padding;
      int aoffset; // aligned offset
      const char *name;
      struct node *val;
    } var;

    struct node_tenary {
      // exp ? true : false
      struct node *true_node;
      struct node *false_node;
    } tenary;

    struct var_list {
      // list of struct node* variables
      struct vector *list;
    } var_list;

    struct bracket {
      // int x[50]; [50] is bracket node and the inner node is NODE_TYPE_NUMBER
      // with value 50
      struct node *inner;
    } bracket;

    struct _struct {
      const char *name;
      struct node *body_n;
      // struct abc {} var_name; var_name is the variable name
      struct node *var;
    } _struct;

    struct _union {
      const char *name;
      struct node *body_n;
      // union abc {} var_name; var_name is the variable name
      struct node *var;
    } _union;

    struct body {
      // vector of struct node*
      struct vector *statements;

      // sum of all the sizes of the variables in the body
      size_t size;

      // true if the variable size had to be increased to align to 16 bytes
      bool padded;

      // pointer to the largest variable node in the body based on size
      struct node *largest_variable_node;
    } body;

    struct function {
      // special flags for function nodes
      int flags;

      // return type of the function
      struct datatype rtype;

      // function name
      const char *name;

      struct function_args {
        // vector of struct node* (variable nodes)
        struct vector *args;

        // how much to add to EBP to get to the first argument
        size_t stack_addition;
      } args;

      // body of the function (NULL if it is a function prototype)
      struct node *body_n;

      // stack frame
      struct stack_frame {
        // vector of struct stack_frame_element*
        struct vector *elements;
      } stack_frame;

      // stack size for all the variables in the function
      size_t stack_size;
    } func;

    struct statement {
      struct return_stmt {
        // return expression
        struct node *exp;
      } return_stmt;

      struct if_stmt {
        // if (cond) { body }
        struct node *cond_node;

        // body of the if statement
        struct node *body_node;

        // else if statement
        struct node *next;
      } if_stmt;

      struct else_stmt {
        // else { body }
        struct node *body_node;
      } else_stmt;

      struct for_stmt {
        // for (init; cond; inc) { body }
        struct node *init;

        // condition
        struct node *cond;

        // increment
        struct node *inc;

        // body of the for statement
        struct node *body;
      } for_stmt;

      struct while_stmt {
        // while (cond) { body }
        struct node *cond;

        // body of the while statement
        struct node *body;
      } while_stmt;

      struct do_while_stmt {
        // do { body } while (cond)
        struct node *cond;

        // body of the do while statement
        struct node *body;
      } do_while_stmt;

      struct switch_stmt {
        // switch (exp) { body }
        struct node *exp;

        // body of the switch statement
        struct node *body;

        // vector of struct parsed_switch_case*
        struct vector *cases;

        // has default case
        bool has_default_case;
      } switch_stmt;

      struct _case_stmt {
        // case 50:
        struct node *exp;
      } _case;

      struct _goto_stmt {
        // goto label;
        struct node *label;
      } _goto;
    } stmt;

    struct node_label {
      // label name
      struct node *name;
    } label;

    struct cast {
      // (type) exp
      struct datatype type;
      struct node *exp;
    } cast;

    struct unary unary;
  };

  union {
    char cval;
    const char *sval;
    unsigned int inum;
    unsigned long lnum;
    unsigned long long llnum;
  };
};

enum {
  RESOLVER_ENTITY_FLAG_IS_STACK = 0b00000001,
  RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY = 0b00000010,
  RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY = 0b00000100,
  RESOLVER_ENTITY_FLAG_DO_INDIRECTION = 0b00001000,
  RESOLVER_ENTITY_FLAG_JUST_USE_OFFSET = 0b00010000,
  RESOLVER_ENTITY_FLAG_IS_POINTER_ARRAY_ENTITY = 0b00100000,
  RESOLVER_ENTITY_FLAG_WAS_CASTED = 0b01000000,
  RESOLVER_ENTITY_FLAG_USES_ARRAY_BRACKETS = 0b10000000,
};

enum {
  RESOLVER_ENTITY_TYPE_VARIABLE,
  RESOLVER_ENTITY_TYPE_FUNCTION,
  RESOLVER_ENTITY_TYPE_STRUCT,
  RESOLVER_ENTITY_TYPE_FUNCTION_CALL,
  RESOLVER_ENTITY_TYPE_ARRAY_BRACKET,
  RESOLVER_ENTITY_TYPE_RULE,
  RESOLVER_ENTITY_TYPE_GENERAL,
  RESOLVER_ENTITY_TYPE_UNARY_GET_ADDRESS,
  RESOLVER_ENTITY_TYPE_UNARY_INDIRECTION,
  RESOLVER_ENTITY_TYPE_UNSUPPORTED,
  RESOLVER_ENTITY_TYPE_CAST,
};

enum {
  RESOLVER_SCOPE_FLAG_IS_STACK = 0b00000001,
};

struct resolver_process;
struct resolver_result;
struct resolver_scope;
struct resolver_entity;

typedef void *(*RESOLVER_NEW_ARRAY_BRACKET_ENTITY)(
    struct resolver_result *result, struct node *array_entity_node);
typedef void (*RESOLVER_DELETE_SCOPE)(struct resolver_scope *scope);
typedef void (*RESOLVER_DELETE_ENTITY)(struct resolver_entity *entity);
typedef struct resolver_entity *(*RESOLVER_MERGE_ENTITIES)(
    struct resolver_process *process, struct resolver_result *result,
    struct resolver_entity *left_entity, struct resolver_entity *right_entity);
typedef void *(*RESOLVER_MAKE_PRIVATE)(struct resolver_entity *entity,
                                       struct node *node, int offset,
                                       struct resolver_scope *scope);
typedef void (*RESOLVER_SET_RESULT_BASE)(struct resolver_result *result,
                                         struct resolver_entity *base_entity);

struct resolver_callbacks {
  RESOLVER_NEW_ARRAY_BRACKET_ENTITY new_array_entity;
  RESOLVER_DELETE_SCOPE delete_scope;
  RESOLVER_DELETE_ENTITY delete_entity;
  RESOLVER_MERGE_ENTITIES merge_entities;
  RESOLVER_MAKE_PRIVATE make_private;
  RESOLVER_SET_RESULT_BASE set_result_base;
};

struct resolver_process {
  // resolver scopes
  struct resolver_scopes {
    // root scope
    struct resolver_scope *root;

    // current scope
    struct resolver_scope *current;
  } scopes;

  // compile process
  struct compile_process *compile_process;

  // resolver callbacks
  struct resolver_callbacks callbacks;
};

struct resolver_array_data {
  // vector of struct resolver_entity*
  struct vector *entities;
};

enum {
  RESOLVER_DEFAULT_ENTITY_TYPE_STACK,
  RESOLVER_DEFAULT_ENTITY_TYPE_SYMBOL
};

enum { RESOLVER_DEFAULT_ENTITY_FLAG_IS_LOCAL_STACK = 0b00000001 };

enum {
  RESOLVER_DEFAULT_ENTITY_DATA_TYPE_VARIABLE,
  RESOLVER_DEFAULT_ENTITY_DATA_TYPE_FUNCTION,
  RESOLVER_DEFAULT_ENTITY_DATA_TYPE_ARRAY_BRACKET,
};

struct resolver_default_entity_data {
  // data type of the entity
  int type;

  // address of the entity
  char address[60];

  // base address of the entity
  char base_address[60];

  // offset from the base
  int offset;

  // flags for the entity
  int flags;
};

struct resolver_default_scope_data {
  int flags;
};

enum {
  RESOLVER_RESULT_FLAG_FAILED = 0b00000001,
  RESOLVER_RESULT_FLAG_RUNTIME_NEEDED_TO_FINISH_PATH = 0b00000010,
  RESOLVER_RESULT_FLAG_PROCESSING_ARRAY_ENTITIES = 0b00000100,
  RESOLVER_RESULT_FLAG_HAS_POINTER_ARRAY_ACCESS = 0b00001000,
  RESOLVER_RESULT_FLAG_FIRST_ENTITY_LOAD_TO_EBX = 0b00010000,
  RESOLVER_RESULT_FLAG_FIRST_ENTITY_PUSH_VALUE = 0b00100000,
  RESOLVER_RESULT_FLAG_FINAL_INDIRECTION_REQUIRED_FOR_VALUE = 0b01000000,
  RESOLVER_RESULT_FLAG_DOES_GET_ADDRESS = 0b10000000,
};

struct resolver_result {
  // first entity in the result
  struct resolver_entity *first_entity_const;

  // enitity identifier that represents the variable at the start of expression
  struct resolver_entity *identifier;

  // last struct or union entity that was discovered
  struct resolver_entity *last_struct_union_entity;

  // array data
  struct resolver_array_data array_data;

  // root entity
  struct resolver_entity *entity;

  // last entity
  struct resolver_entity *last_entity;

  // flags for the result
  int flags;

  // total number of entities in the result
  size_t count;

  // result base
  struct resolver_result_base {
    // address of the base
    char address[60];

    // base address
    char base_address[60];

    // offset from the base
    int offset;
  } base;
};

struct resolver_scope {
  // flags for the scope
  int flags;

  // vector of struct resolver_entity*
  struct vector *entities;

  // next scope
  struct resolver_scope *next;

  // previous scope
  struct resolver_scope *prev;

  // private data for the scope
  void *private;
};

struct resolver_entity {
  // flags for the entity
  int flags;

  // type of the entity
  int type;

  // name of the entity
  const char *name;

  // offset from the base pointer
  int offset_from_bp;

  // node that the entity is related to
  struct node *node;

  union {
    // data for the variable entity
    struct resolver_entity_var_data {
      // data type of the variable
      struct datatype *type;

      // array runtime data
      struct resolver_array_runtime {
        // data type of the array
        struct datatype *type;

        // node to index the array
        struct node *index_node;

        // multiplier for the index
        size_t multiplier;
      } array_runtime;
    } var_data;

    // array
    struct resolver_array {
      // data type of the array
      struct datatype type;

      // array index node
      struct node *array_index_node;

      // index of the array
      int index;
    } array;

    // function call data
    struct resolver_entity_function_call_data {
      // vector of struct node* arguments
      struct vector *args;

      // stack size for the function call
      size_t stack_size;
    } function_call_data;

    // data for the rule entity
    struct resolver_entity_rule {
      // entity to the left
      struct resolver_entity_rule_left {
        // flags for the left entity
        int flags;
      } left;

      // entity to the right
      struct resolver_entity_rule_right {
        // flags for the right entity
        int flags;
      } right;
    } rule;

    // indirection data
    struct resolver_indirection {
      // depth of the indirection
      int depth;
    } indirection;
  };

  // last resolver entity
  struct entity_last_resolve {
    // reference to the node that the entity is related to
    struct node *referencing_node;
  } last_resolve;

  // data type of the entity
  struct datatype dtype;

  // scope of the entity
  struct resolver_scope *scope;

  // result of the entity resolution
  struct resolver_result *result;

  // process that the entity is related to
  struct resolver_process *process;

  // private data for the entity
  void *private;

  // next entity
  struct resolver_entity *next;

  // previous entity
  struct resolver_entity *prev;
};

enum {
  DATATYPE_FLAG_IS_SIGNED = 0b00000001,
  DATATYPE_FLAG_IS_STATIC = 0b00000010,
  DATATYPE_FLAG_IS_CONST = 0b00000100,
  DATATYPE_FLAG_IS_POINTER = 0b00001000,
  DATATYPE_FLAG_IS_ARRAY = 0b00010000,
  DATATYPE_FLAG_IS_EXTERN = 0b00100000,
  DATATYPE_FLAG_IS_RESTRICT = 0b01000000,
  DATATYPE_FLAG_IGNORE_TYPECHECK = 0b10000000,
  DATATYPE_FLAG_IS_SECONDARY = 0b100000000,
  DATATYPE_FLAG_STRUCT_UNION_NO_NAME = 0b1000000000,
  DATATYPE_FLAG_IS_LITERAL = 0b10000000000,
};

enum {
  DATA_TYPE_VOID,
  DATA_TYPE_CHAR,
  DATA_TYPE_SHORT,
  DATA_TYPE_INT,
  DATA_TYPE_LONG,
  DATA_TYPE_FLOAT,
  DATA_TYPE_DOUBLE,
  DATA_TYPE_STRUCT,
  DATA_TYPE_UNION,
  DATA_TYPE_UNKNOWN,
};

enum {
  DATA_TYPE_EXPECT_PRIMITIVE,
  DATA_TYPE_EXPECT_UNION,
  DATA_TYPE_EXPECT_STRUCT,
};

enum {
  DATA_SIZE_ZERO = 0,
  DATA_SIZE_BYTE = 1,
  DATA_SIZE_WORD = 2,
  DATA_SIZE_DWORD = 4,
  DATA_SIZE_QWORD = 8,
};

enum {
  EXPRESSION_FLAG_RIGHT_NODE = 0b0000000000000001,
  EXPRESSION_IN_FUNCTION_CALL_ARGUMENTS = 0b0000000000000010,
  EXPRESSION_IN_FUNCTION_CALL_LEFT_OPERAND = 0b0000000000000100,
  EXPRESSION_IS_ADDITION = 0b0000000000001000,
  EXPRESSION_IS_SUBTRACTION = 0b0000000000010000,
  EXPRESSION_IS_MULTIPLICATION = 0b0000000000100000,
  EXPRESSION_IS_DIVISION = 0b0000000001000000,
  EXPRESSION_IS_FUNCTION_CALL = 0b0000000010000000,
  EXPRESSION_INDIRECTION = 0b0000000100000000,
  EXPRESSION_GET_ADDRESS = 0b0000001000000000,
  EXPRESSION_IS_ABOVE = 0b0000010000000000,
  EXPRESSION_IS_ABOVE_OR_EQUAL = 0b0000100000000000,
  EXPRESSION_IS_BELOW = 0b0001000000000000,
  EXPRESSION_IS_BELOW_OR_EQUAL = 0b0010000000000000,
  EXPRESSION_IS_EQUAL = 0b0100000000000000,
  EXPRESSION_IS_NOT_EQUAL = 0b1000000000000000,
  EXPRESSION_LOGICAL_AND = 0b10000000000000000,
  EXPRESSION_LOGICAL_OR = 0b100000000000000000,
  EXPRESSION_IS_IN_LOGICAL_EXPRESSION = 0b1000000000000000000,
  EXPRESSION_IS_BITSHIFT_LEFT = 0b10000000000000000000,
  EXPRESSION_IS_BITSHIFT_RIGHT = 0b100000000000000000000,
  EXPRESSION_IS_BITWISE_OR = 0b1000000000000000000000,
  EXPRESSION_IS_BITWISE_AND = 0b10000000000000000000000,
  EXPRESSION_IS_BITWISE_XOR = 0b100000000000000000000000,
  EXPRESSION_IS_NOT_ROOT_NODE = 0b1000000000000000000000000,
  EXPRESSION_IS_ASSIGNMENT = 0b10000000000000000000000000,
  IS_ALONE_STATEMENT = 0b100000000000000000000000000,
  EXPRESSION_IS_UNARY = 0b1000000000000000000000000000,
  IS_STATEMENT_RETURN = 0b10000000000000000000000000000,
  IS_RIGHT_OPERAND_OF_ASSIGNMENT = 0b100000000000000000000000000000,
  IS_LEFT_OPERAND_OF_ASSIGNMENT = 0b1000000000000000000000000000000,
  EXPRESSION_IS_MODULUS = 0b10000000000000000000000000000000,
};

enum {
  STRUCT_ACCESS_BACKWARDS = 0b00000001,
  STRUCT_STOP_AT_POINTER_ACCESS = 0b00000010,
};

enum {
  FUNCTION_NODE_FLAG_IS_NATIVE = 0b00000001,
};

int compile_file(const char *filename, const char *out_filename, int flags);
struct compile_process *compile_process_create(const char *filename,
                                               const char *filename_out,
                                               int flags);

char compile_process_next_char(struct lex_process *lex_process);
char compile_process_peek_char(struct lex_process *lex_process);
void compile_process_push_char(struct lex_process *lex_process, char c);

void compiler_error(struct compile_process *compiler, const char *msg, ...);
void compiler_warning(struct compile_process *compiler, const char *msg, ...);

struct lex_process *lex_process_create(struct compile_process *compiler,
                                       struct lex_process_functions *functions,
                                       void *private);
void lex_process_free(struct lex_process *process);
void *lex_process_private(struct lex_process *process);
struct vector *lex_process_tokens(struct lex_process *process);
int lex(struct lex_process *process);
int parse(struct compile_process *process);
/**
 * builds tokens for the input string
 */
struct lex_process *tokens_build_for_string(struct compile_process *compiler,
                                            const char *str);

bool token_is_identifier(struct token *token);
bool token_is_keyword(struct token *token, const char *value);
bool token_is_nl_or_comment_or_newline_separator(struct token *token);
bool token_is_symbol(struct token *token, char c);
bool keyword_is_datatype(const char *keyword);
bool token_is_primitive_keyword(struct token *token);
bool token_is_operator(struct token *token, const char *value);

bool datatype_is_struct_or_union_for_name(const char *name);
bool datatype_is_struct_or_union(struct datatype *dtype);
bool datatype_is_primitive(struct datatype *dtype);
bool datatype_is_struct_or_union_no_pointer(struct datatype *dtype);
struct datatype datatype_for_numeric();

bool is_access_operator(const char *op);
bool is_access_node(struct node *node);
bool is_array_operator(const char *op);
bool is_array_node(struct node *node);
bool is_parentheses_operator(const char *op);
bool is_parentheses_node(struct node *node);
bool is_access_node_with_op(struct node *node, const char *op);
bool is_argument_operator(const char *op);
bool is_argument_node(struct node *node);
void datatype_decrement_pointer(struct datatype *dtype);

size_t datatype_size_for_array_access(struct datatype *dtype);
size_t datatype_element_size(struct datatype *dtype);
size_t datatype_size_no_ptr(struct datatype *dtype);
size_t datatype_size(struct datatype *dtype);

struct node *node_create(struct node *_node);
void make_exp_node(struct node *left_node, struct node *right_node,
                   const char *op);
void make_exp_parentheses_node(struct node *exp_node);
void make_bracket_node(struct node *node);
void make_body_node(struct vector *body_vec, size_t size, bool padded,
                    struct node *largest_var_node);
void make_struct_node(const char *name, struct node *body_node);
void make_union_node(const char *name, struct node *body_node);
void make_function_node(struct datatype *rtype, const char *name,
                        struct vector *args, struct node *body);
void make_if_node(struct node *cond_node, struct node *body_node,
                  struct node *next_node);
void make_else_node(struct node *body_node);
void make_return_node(struct node *exp_node);
void make_for_node(struct node *init_node, struct node *cond_node,
                   struct node *inc_node, struct node *body_node);
void make_while_node(struct node *cond_node, struct node *body_node);
void make_do_while_node(struct node *cond_node, struct node *body_node);
void make_switch_node(struct node *exp_node, struct node *body_node,
                      struct vector *cases, bool has_default_case);
void make_continue_node();
void make_break_node();
void make_label_node(struct node *name_node);
void make_goto_node(struct node *label_node);
void make_case_node(struct node *exp_node);
void make_tenary_node(struct node *true_node, struct node *false_node);
void make_cast_node(struct datatype *type, struct node *exp_node);
void make_unary_node(const char *op, struct node *operand_node);

struct node *node_pop();
struct node *node_peek();
struct node *node_peek_or_null();
void node_push(struct node *node);
void node_set_vector(struct vector *vec, struct vector *root_vec);
struct node *node_create(struct node *_node);
struct node *struct_node_for_name(struct compile_process *process,
                                  const char *name);
struct node *union_node_for_name(struct compile_process *process,
                                 const char *name);
struct node *node_from_symbol(struct compile_process *process,
                              const char *name);
struct node *node_from_sym(struct symbol *sym);
struct node *node_peek_expressionable_or_null();
struct node *variable_struct_or_union_body_node(struct node *node);
struct node *variable_node(struct node *node);
struct node *variable_node_or_list(struct node *node);
bool variable_node_is_primitive(struct node *node);
bool node_is_struct_or_union(struct node *node);
bool node_is_struct_or_union_variable(struct node *node);
bool node_is_expression_or_parentheses(struct node *node);
bool node_is_value_type(struct node *node);
bool node_is_expression(struct node *node, const char *op);
bool is_array_node(struct node *node);
bool is_node_assignment(struct node *node);
bool is_unary_operator(const char *op);
bool op_is_indirection(const char *op);
bool op_is_address(const char *op);
bool node_valid(struct node *node);
bool function_node_is_prototype(struct node *node);

size_t function_node_stack_size(struct node *node);
size_t function_node_argument_stack_addition(struct node *node);
struct vector *function_node_argument_vec(struct node *node);

struct array_brackets *array_brackets_new();
void array_brackets_free(struct array_brackets *brackets);
void array_brackets_add(struct array_brackets *brackets,
                        struct node *bracket_node);
struct vector *array_brackets_node_vector(struct array_brackets *brackets);
size_t array_brackets_calculate_size_from_index(struct datatype *dtype,
                                                struct array_brackets *brackets,
                                                int index);
size_t array_brackets_calculate_size(struct datatype *dtype,
                                     struct array_brackets *brackets);
int array_total_indexes(struct datatype *dtype);
size_t array_brackets_count(struct datatype *dtype);

struct scope *scope_new(struct compile_process *process, int flags);
struct scope *scope_create_root(struct compile_process *process);
void scope_free_root(struct compile_process *process);
void scope_iteration_start(struct scope *scope);
void scope_iteration_end(struct scope *scope);
void *scope_last_entity_at_scope(struct scope *scope);
void *scope_last_entity_from_scope_stop_at(struct scope *scope,
                                           struct scope *stop_scope);
void *scope_last_entity_stop_at(struct compile_process *process,
                                struct scope *stop_scope);
void *scope_last_entity(struct compile_process *process);
void scope_push(struct compile_process *process, void *ptr, size_t elem_size);
void scope_finish(struct compile_process *process);
struct scope *scope_current(struct compile_process *process);

// get the size of a variable node
size_t variable_size(struct node *var_node);
// get the sum of all the sizes of the variables in a variable list node
size_t variable_size_for_list(struct node *var_list_node);
// get the padding needed for a variable to align to 16 bytes
int padding(int val, int to);
// get the value aligned to 16 bytes
int align_value(int val, int to);
int align_value_treat_positive(int val, int to);
// get the sum of all the paddings of the variables in a vector
int compute_sum_padding(struct vector *vec);
int array_multiplier(struct datatype *dtype, int index, int index_val);
int array_offset(struct datatype *dtype, int index, int index_val);
int struct_offset(struct compile_process *compile_process,
                  const char *struct_name, const char *var_name,
                  struct node **var_node_out, int last_pos, int flags);
struct node *
variable_struct_or_union_largest_variable_node(struct node *var_node);
struct node *body_larest_variable_node(struct node *body_node);

void symresolver_init(struct compile_process *process);
void symresolver_new_table(struct compile_process *process);
void symresolver_end_table(struct compile_process *process);
void symresolver_build_for_node(struct compile_process *process,
                                struct node *node);
struct symbol *symresolver_get_symbol(struct compile_process *process,
                                      const char *name);
struct symbol *
symresolver_get_symbol_for_native_function(struct compile_process *process,
                                           const char *name);

#define TOTAL_OPERATOR_GROUPS 14
#define MAX_OPERATORS_IN_GROUP 12

enum { ASSOCIATIVITY_LEFT_TO_RIGHT, ASSOCIATIVITY_RIGHT_TO_LEFT };

struct expressionable_op_precedence_group {
  char *operators[MAX_OPERATORS_IN_GROUP];
  int associativity;
};

struct fixup;

// return true if the fixup is done
typedef bool (*FIXUP_FIX)(struct fixup *fixup);

// signify the end of the fixup and free any resources
typedef void (*FIXUP_END)(struct fixup *fixup);

struct fixup_config {
  FIXUP_FIX fix;
  FIXUP_END end;
  void *private;
};

struct fixup_system {
  struct vector *fixups;
};

enum {
  FIXUP_FLAG_RESOLVED = 0b00000001,
};

struct fixup {
  int flags;
  struct fixup_system *system;
  struct fixup_config config;
};

struct fixup_system *fixup_sys_new();
struct fixup_config *fixup_config(struct fixup *fixup);
void fixup_free(struct fixup *fixup);
void fixup_start_iteration(struct fixup_system *system);
struct fixup *fixup_next(struct fixup_system *system);
void fixup_sys_fixups_free(struct fixup_system *system);
void fixup_sys_free(struct fixup_system *system);
int fixup_sys_unresolved_count(struct fixup_system *system);
struct fixup *fixup_register(struct fixup_system *system,
                             struct fixup_config *config);
bool fixup_resolve(struct fixup *fixup);
void *fixup_private(struct fixup *fixup);
bool fixups_resolve(struct fixup_system *system);

struct resolver_entity *resolver_make_entity(
    struct resolver_process *process, struct resolver_result *result,
    struct datatype *custom_dtype, struct node *node,
    struct resolver_entity *guided_entity, struct resolver_scope *scope);
struct resolver_process *
resolver_new_process(struct compile_process *compile_process,
                     struct resolver_callbacks *callbacks);
struct resolver_scope *resolver_new_scope(struct resolver_process *process,
                                          void *private, int flags);
void resolver_finish_scope(struct resolver_process *process);
struct resolver_entity *
resolver_register_function(struct resolver_process *process,
                           struct node *func_node, void *private);
struct resolver_entity *
resolver_new_entity_for_var_node(struct resolver_process *process,
                                 struct node *var_node, void *private,
                                 int offset);
struct resolver_process *
resolver_default_new_process(struct compile_process *compile_process);
struct resolver_entity *resolver_default_merge_entities(
    struct resolver_process *process, struct resolver_result *result,
    struct resolver_entity *left_entity, struct resolver_entity *right_entity);
void resolver_default_delete_scope(struct resolver_scope *scope);
void resolver_default_delete_entity(struct resolver_entity *entity);
void *resolver_default_new_array_entity(struct resolver_result *result,
                                        struct node *array_entity_node);
void resolver_default_finish_scope(struct resolver_process *process);
void resolver_default_new_scope(struct resolver_process *process, int flags);
struct resolver_entity *
resolver_default_register_function(struct resolver_process *process,
                                   struct node *func_node, int flags);
struct resolver_entity *
resolver_default_new_scope_entity(struct resolver_process *process,
                                  struct node *var_node, int offset, int flags);
struct resolver_default_entity_data *
resolver_default_new_entity_data_for_function(struct node *func_node,
                                              int flags);
struct resolver_default_entity_data *
resolver_default_new_entity_data_for_array_bracket(struct node *bracket_node);
struct resolver_default_entity_data *
resolver_default_new_entity_for_var_node(struct node *var_node, int offset,
                                         int flags);
void resolver_default_set_result_base(struct resolver_result *result,
                                      struct resolver_entity *base_entity);
void *resolver_default_make_private(struct resolver_entity *entity,
                                    struct node *node, int offset,
                                    struct resolver_scope *scope);
void resolver_default_entity_data_set_address(
    struct resolver_default_entity_data *entity_data, struct node *var_node,
    int offset, int flags);
void resolver_default_global_asm_address(const char *name, int offset,
                                         char *address_out);
struct resolver_default_entity_data *resolver_default_new_entity_data();
char *resolver_default_stack_asm_address(int stack_offset, char *out);
struct resolver_default_scope_data *
resolver_default_scope_private(struct resolver_scope *scope);
struct resolver_default_entity_data *
resolver_default_entity_private(struct resolver_entity *entity);

int codegen(struct compile_process *process);
struct code_generator *codegenerator_new(struct compile_process *process);

#endif
