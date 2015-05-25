#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <float.h>
#include <math.h>

#include "scriptlet.h"
#include "skipbuck_string.h"
#include "thread.h"

#ifdef _WIN32
#define isnan _isnan
#endif

#define STACK_SIZE (1024*8)		// nbr of elements
#define TOKEN_SIZE 256			// biggest string literal
#define DEBUG if (0)

typedef char token[TOKEN_SIZE];

static const struct precedence_table_
{
	const char *op;
	int precedence;
}
 ptable[] =
{
	{"(", 1},
	{")", 1},
	{";", 1},
	{"=", 2},

	{"||", 2},
	{"^^", 3},
	{"&&", 4},
	{"!", 5},

	{"+", 10},
	{"-", 10},
	{"*", 11},
	{"**", 11},
	{"/", 11},
	{"%", 11},

	{"<<", 11},
	{">>", 11},
	{">>>", 11},

	{"&", 20},
	{"|", 20},
	{"^", 20},
	{"~", 20},

	{"!=", 30},
	{"<", 30},
	{"<=", 30},
	{"==", 30},
	{">=", 30},
	{">", 30},

	{"fold-case", 10},
	{"equals", 10},
	{"contains", 10},
	{"begins-with", 10},
	{"ends-with", 10},

	{"int", 10},
	{"real", 10},
	{"nan", 10},
	{"string", 10},
	{"size", 10},

	{"if", 10},
	{"else", 10},
	{"fi", 10},

	{"jday", 10},
	{"dow", 10},
	{"print", 10},

	{0, 0}
};

typedef enum
{
    empty_tc,
    name_tc,
    int_tc,
    real_tc,
    string_tc,
    eq_tc,
    neq_tc,
    lt_tc,
    leq_tc,
    gt_tc,
    geq_tc,
    or_tc,
    xor_tc,
    and_tc,
    not_tc,
    unary_plus_tc,
    unary_minus_tc,
    multiply_tc,
    power_tc,
    divide_tc,
    modulo_tc,
    add_tc,
    subtract_tc,
    shift_left_tc,
    shift_right_tc,
    logical_shift_right_tc,
    bit_and_tc,
    bit_xor_tc,
    bit_or_tc,
    bit_negate_tc,

    func_is_int_tc,
    func_is_real_tc,
    func_is_string_tc,
    func_is_nan_tc,
    func_begins_with_tc,
    func_contains_tc,
    func_ends_with_tc,
    func_fold_case_tc,
    func_eq_tc,
    func_size_tc,

	func_print_tc,
	func_col_tc,
	func_jday_tc,
	func_dow_tc,

	assign_tc,
	if_tc,
	else_tc,
	fi_tc,
	stack_tc
}
 typecode;

struct bytecode_
{
	int level, nbr_params;
	typecode tc;

	union
	{
		long long int_val;
		double real_val;
		token str_val;
	};
};

typedef struct bytecode_ *bytecode;

struct item_
{
	token tok;
	int precedence, save_operands;
};

struct scriptlet_
{
	int it_codes, use_cnt;
	struct bytecode_ codes[STACK_SIZE];
};

struct compiletime_
{
	scriptlet *s;
	int it_operands, it_ops, level;
	struct item_ operands[STACK_SIZE], ops[STACK_SIZE];
};

typedef struct compiletime_ *compiletime;

struct hscriptlet_
{
	scriptlet *s;
	skipbuck *symtab;
	struct bytecode_ stack[STACK_SIZE], syms[STACK_SIZE];
	int it_stack, it_syms, it_syms_save, it, level;
};

/* Valid after Jan 1, 1700 */

static int juliandate(int year, int month, int day)
{
	year -= 1700 + (month < 3);

	return (365L  *(year + (month < 3)) + year / 4 - year / 100 +
		(year + 100) / 400 + (305  *(month - 1) - (month > 2)  *20 +
			(month > 7)  *5 + 5) / 10 + day + 2341972L);
}

/* 1 = Mon, 2 = Tue etc...  7 = Sun */

static int dayofweek(int jdate)
{
	return (jdate % 7) + 1;
}

static int ends_with(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return 0;

    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);

    if (lensuffix > lenstr)
        return 0;

    return !strcmp(str+lenstr-lensuffix, suffix);
}

static void scriptlet_share(scriptlet *s)
{
	if (!s)
		return;

	atomic_inc(&s->use_cnt);
}

static void scriptlet_unshare(scriptlet *s)
{
	if (!s)
		return;

	if (!atomic_dec(&s->use_cnt))
		free(s);
}

hscriptlet *scriptlet_prepare(scriptlet *s)
{
	scriptlet_share(s);
	hscriptlet *r = (hscriptlet*)calloc(1, sizeof(struct hscriptlet_));
	r->s = s;
	r->symtab = sb_string_create();
	scriptlet_set_int(r, "$FIRST", 1);
	r->it_syms_save = r->it_syms;
	return r;
}

int scriptlet_done(hscriptlet *r)
{
	scriptlet_unshare(r->s);
	sb_string_destroy(r->symtab);
	free(r);
	return 1;
}

int scriptlet_set_int(hscriptlet *r, const char *k, long long value)
{
	bytecode code = &r->syms[r->it_syms++];
	code->tc = int_tc;
	code->int_val = value;
	sb_string_del(r->symtab, k);
	sb_string_set(r->symtab, k, code);
	return 1;
}

int scriptlet_set_real(hscriptlet *r, const char *k, double value)
{
	bytecode code = &r->syms[r->it_syms++];
	code->tc = real_tc;
	code->real_val = value;
	sb_string_del(r->symtab, k);
	sb_string_set(r->symtab, k, code);
	return 1;
}

int scriptlet_set_string(hscriptlet *r, const char *k, const char *value)
{
	bytecode code = &r->syms[r->it_syms++];
	code->tc = string_tc;
	strcpy(code->str_val, value);
	sb_string_del(r->symtab, k);
	sb_string_set(r->symtab, k, code);
	return 1;
}

static bytecode substitute(const hscriptlet *r, const bytecode v)
{
	bytecode code = v;

	if (v->tc == string_tc)
		sb_string_get(r->symtab, v->str_val, &code);

	return code;
}

int scriptlet_get_int(hscriptlet *r, const char *k, long long *value)
{
	bytecode code = NULL;

	if (!sb_string_get(r->symtab, k, &code))
		return 0;

	if (code->tc == int_tc)
		*value = (long long)code->int_val;
	else if (code->tc == real_tc)
		*value = (long long)code->real_val;
	else
		return 0;

	return 1;
}

int scriptlet_get_real(hscriptlet *r, const char *k, double *value)
{
	bytecode code = NULL;

	if (!sb_string_get(r->symtab, k, &code))
		return 0;

	if (code->tc == int_tc)
		*value = (double)code->int_val;
	else if (code->tc == real_tc)
		*value = (double)code->real_val;
	else
		return 0;

	return 1;
}

int scriptlet_get_string(hscriptlet *r, const char *k, const char **value)
{
	bytecode code = NULL;

	if (!sb_string_get(r->symtab, k, &code))
		return 0;

	if (code->tc == string_tc)
		*value = code->str_val;
	else
		return 0;

	return 1;
}

static int get_precedence(const char *tok)
{
	const struct precedence_table_ *ptr = ptable;

	while (ptr->op)
	{
		if (!strcmp(tok, ptr->op))
			return ptr->precedence;

		ptr++;
	}

	return 0;
}

static void push_operand(compiletime c, const char *tok)
{
	strcpy(c->operands[c->it_operands].tok, tok);
	c->operands[c->it_operands].precedence = 0;
	c->it_operands++;
}

static const char *pop_operand(compiletime c)
{
	if (!c->it_operands)
		return NULL;

	c->it_operands--;
	return c->operands[c->it_operands].tok;
}


static void push_operator(compiletime c, const char *tok, int precedence)
{
	strcpy(c->ops[c->it_ops].tok, tok);
	c->ops[c->it_ops].precedence = precedence;
	c->ops[c->it_ops].save_operands = c->it_operands;
	c->it_ops++;
}

static const char *head_operator(compiletime c, int *precedence, int *empty)
{
	if (!c->it_ops)
	{
		*empty = 1;
		return NULL;
	}

	*empty = 0;
	*precedence = c->ops[c->it_ops-1].precedence;
	return c->ops[c->it_ops-1].tok;
}

static const char *pop_operator(compiletime c, int *operand_count)
{
	if (!c->it_ops)
		return NULL;

	--c->it_ops;
	*operand_count = (c->it_operands - c->ops[c->it_ops].save_operands) + 1;
	return c->ops[c->it_ops].tok;
}

static typecode get_typecode(const char *tok)
{
	const char *src = tok;
	int is_numeric = 1;
	int is_real = 0;
	int is_exponent = 0;

	if (*src == '-')
		src++;

	while (*src)
	{
		if (*src == '.')
			is_real = 1;
		else if (is_real && ((*src == 'e') || (*src == 'E')))
			is_exponent = 1;
		else if (is_exponent && ((*src == '-') || (*src == '+')))
			;
		else if (!isdigit(*src))
			is_numeric = 0;

		src++;
	}

	if (is_numeric && is_real)
		return real_tc;
	else if (is_numeric)
		return int_tc;
	else if (!strcmp(tok, "_STACK"))
		return stack_tc;
	else
		return string_tc;
}

static void emit_code(compiletime c, typecode tc, int nbr_params)
{
	bytecode code = &c->s->codes[c->s->it_codes++];
	code->level = c->level;
	code->nbr_params = nbr_params;
	code->tc = tc;
}

static void emit_value(compiletime c, const char *v)
{
	bytecode code = &c->s->codes[c->s->it_codes++];
	code->level = c->level;
	code->nbr_params = 0;
	code->tc = get_typecode(v);

	if (code->tc == real_tc)
		code->real_val = atof(v);
	else if (code->tc == int_tc)
	{
		long long tmp = 0;
		sscanf(v, "%lld", &tmp);
		code->int_val = tmp;
	}
	else if (code->tc == string_tc)
		strcpy(code->str_val, v);
}

static void emit1(compiletime c, typecode tc)
{
	emit_code(c, tc, 0);
}

static void emit2(compiletime c, typecode tc, const char *v1)
{
	emit_code(c, tc, 1);
	emit_value(c, v1);
}

static void emit3(compiletime c, typecode tc, const char *v1, const char *v2)
{
	emit_code(c, tc, 2);
	emit_value(c, v1);
	emit_value(c, v2);
}

static int compile_operator(compiletime c)
{
	int operand_count = 0;

	const char *op = pop_operator(c, &operand_count);

	if (!strcmp(op, "*"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, multiply_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "**"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, power_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "/"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, divide_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "%"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, modulo_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "="))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, assign_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, ";"))
	{
	}
	else if (!strcmp(op, "if"))
	{
		const char *val1 = pop_operand(c);
		emit2(c, if_tc, val1);
		DEBUG printf("emit '%s' %s\n", val1, op);
		c->level++;
	}
	else if (!strcmp(op, "else"))
	{
		c->level--;
		emit1(c, else_tc);
		c->level++;
		DEBUG printf("emit %s\n", op);
	}
	else if (!strcmp(op, "fi"))
	{
		c->level--;
		emit1(c, fi_tc);
		DEBUG printf("emit %s\n", op);
	}
	else if (!strcmp(op, "!"))
	{
		const char *val1 = pop_operand(c);
		emit2(c, not_tc, val1);
		DEBUG printf("emit '%s' %s\n", val1, op);
	}
	else if (!strcmp(op, "+"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, add_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "-"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, subtract_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, ">"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, gt_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, ">="))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, geq_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "<="))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, leq_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "<"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, lt_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "&&"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, and_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "||"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, or_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "^^"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, xor_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "&"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, bit_and_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "|"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, bit_or_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "^"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, bit_xor_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "~"))
	{
		const char *val1 = pop_operand(c);
		emit2(c, bit_negate_tc, val1);
		DEBUG printf("emit %s %s\n", val1, op);
	}
	else if (!strcmp(op, "<<"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, shift_left_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, ">>"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, shift_right_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, ">>>"))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, logical_shift_right_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "=="))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, eq_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "!="))
	{
		const char *val2 = pop_operand(c);
		const char *val1 = pop_operand(c);
		emit3(c, neq_tc, val1, val2);
		DEBUG printf("emit %s %s %s\n", val1, op, val2);
	}
	else if (!strcmp(op, "fold-case"))
	{
		const char *val1 = pop_operand(c);
		emit2(c, func_fold_case_tc, val1);
		DEBUG printf("emit %s %s\n", op, val1);
	}
	else if (!strcmp(op, "size"))
	{
		const char *val1 = pop_operand(c);
		emit2(c, func_size_tc, val1);
		DEBUG printf("emit %s %s\n", op, val1);
	}
	else if (!strcmp(op, "print"))
	{
		const char *val1 = pop_operand(c);
		emit2(c, func_print_tc, val1);
		DEBUG printf("emit %s %s\n", op, val1);
	}
	else if (!strcmp(op, "jday"))
	{
		const char *val1 = pop_operand(c);
		emit2(c, func_jday_tc, val1);
		DEBUG printf("emit %s %s\n", op, val1);
	}
	else if (!strcmp(op, "dow"))
	{
		const char *val1 = pop_operand(c);
		emit2(c, func_dow_tc, val1);
		DEBUG printf("emit %s %s\n", op, val1);
	}
	else if (!strcmp(op, "equals"))
	{
		emit_code(c, func_eq_tc, operand_count);

		while (operand_count--)
		{
			const char *val1 = pop_operand(c);
			emit_value(c, val1);
			DEBUG printf("emit %s '%s'\n", op, val1);
		}
	}
	else if (!strcmp(op, "contains"))
	{
		emit_code(c, func_contains_tc, operand_count);

		while (operand_count--)
		{
			const char *val1 = pop_operand(c);
			emit_value(c, val1);
			DEBUG printf("emit %s '%s'\n", op, val1);
		}
	}
	else if (!strcmp(op, "begins-with"))
	{
		emit_code(c, func_begins_with_tc, operand_count);

		while (operand_count--)
		{
			const char *val1 = pop_operand(c);
			emit_value(c, val1);
			DEBUG printf("emit %s '%s'\n", op, val1);
		}
	}
	else if (!strcmp(op, "ends-with"))
	{
		emit_code(c, func_ends_with_tc, operand_count);

		while (operand_count--)
		{
			const char *val1 = pop_operand(c);
			emit_value(c, val1);
			DEBUG printf("emit %s '%s'\n", op, val1);
		}
	}
	else if (!strcmp(op, "int"))
	{
		const char *val1 = pop_operand(c);
		emit2(c, func_is_int_tc, val1);
		DEBUG printf("emit %s '%s'\n", op, val1);
	}
	else if (!strcmp(op, "real"))
	{
		const char *val1 = pop_operand(c);
		emit2(c, func_is_real_tc, val1);
		DEBUG printf("emit %s '%s'\n", op, val1);
	}
	else if (!strcmp(op, "nan"))
	{
		const char *val1 = pop_operand(c);
		emit2(c, func_is_nan_tc, val1);
		DEBUG printf("emit %s '%s'\n", op, val1);
	}
	else if (!strcmp(op, "string"))
	{
		const char *val1 = pop_operand(c);
		emit2(c, func_is_string_tc, val1);
		DEBUG printf("emit %s '%s'\n", op, val1);
	}
	else if (!strcmp(op, "("))
	{
		return 1;
	}
	else if (!strcmp(op, ")"))
	{
		return 1;
	}
	else
	{
		DEBUG printf("bad operator '%s'\n", op);
		return 0;
	}

	push_operand(c, "_STACK");
	return 1;
}

static const char *get_token(const char *src, char *tok)
{
	char *dst = tok;
	*dst = 0;
	char quoted = 0;

	while ((*src == ' ') || (*src == '\t') || (*src == ',') || (*src == '\n'))
		src++;

	if (!*src)
		return NULL;

	while (*src)
	{
		char ch = *src++;

		if (!quoted && ((ch == '"') || (ch == '\'')))
		{
			quoted = ch;
			continue;
		}

		if (quoted && (ch == quoted))
		{
			quoted = 0;
			break;
		}

		if (!quoted)
		{
			if ((ch == ' ') || (ch == '\t') || (ch == ',') || (ch == '\n'))
				break;
		}

		*dst++ = ch;

		if ((dst - tok) >= (TOKEN_SIZE-1))
			break;

		if (quoted)
			continue;

		if ((ch == '(') || (ch == ')') || (ch == ';'))
			break;

		if ((*src == '(') || (*src == ')') || (*src == ';'))
			break;
	}

	*dst = 0;

	if ((tok[0] == '0') && tok[1] && (tok[1] != '.'))
	{
		const char *src = tok+1;

		if (*src == 'x')
		{
			src++;
			unsigned long long number;
			sscanf(src, "%llx", &number);
			sprintf(tok, "%llu", number);
		}
		else
		{
			unsigned long long number;
			sscanf(src, "%llo", &number);
			sprintf(tok, "%llu", number);
		}

		return src;
	}

	return src;
}

static int compile(scriptlet *s, const char *src)
{
	compiletime c = (compiletime)calloc(1, sizeof(struct compiletime_));
	c->s = s;
	s->it_codes = 0;
	token tok;

	while ((src = get_token(src, tok)) != 0)
	{
		int precedence = get_precedence(tok);

		if (precedence)
		{
			DEBUG printf("operator '%s' precedence=%d\n", tok, precedence);
		}
		else
		{
			DEBUG printf("operand '%s'\n", tok);
		}

		if (!precedence)
		{
			push_operand(c, tok);
			continue;
		}

		if (!strcmp(tok, "("))
		{
			push_operator(c, tok, precedence);
			continue;
		}

		int top_precedence, discard = 0, empty_list;
		const char *top_op;

		while ((top_op = head_operator(c, &top_precedence, &empty_list)), !empty_list)
		{
			if (!strcmp(top_op, "(") && !strcmp(tok, ")"))
			{
				int operand_count;
				pop_operator(c, &operand_count);
				discard = 1;
				continue;
			}

			DEBUG printf("top_op = '%s' with precedence = %d\n", top_op, top_precedence);

			if (precedence > top_precedence)
				break;

			if (!compile_operator(c))
			{
				free(c);
				return 0;
			}

			if (discard)
				break;
		}

		if (!discard)
			push_operator(c, tok, precedence);
	}

	int top_precedence, empty_list;
	const char *top_op;

	while ((top_op = head_operator(c, &top_precedence, &empty_list)), !empty_list )
	{
		DEBUG printf("top_op = '%s'\n", top_op);

		if (!compile_operator(c))
		{
			free(c);
			return 0;
		}
	}

	free(c);
	return 1;
}

static int pop_stack(hscriptlet *r, bytecode *value)
{
	if (!r->it_stack)
		return empty_tc;

	bytecode code = &r->stack[--r->it_stack];

	if (code->tc == int_tc)
		*value = code;
	else if (code->tc == real_tc)
		*value = code;
	else if (code->tc == string_tc)
		*value = code;

	return code->tc;
}

static void push_stack_int(hscriptlet *r, long long value)
{
	bytecode code = &r->stack[r->it_stack++];
	code->tc = int_tc;
	code->int_val = value;
}

static void push_stack_real(hscriptlet *r, double value)
{
	bytecode code = &r->stack[r->it_stack++];
	code->tc = real_tc;
	code->real_val = value;
}

static void push_stack_string(hscriptlet *r, const char *value)
{
	bytecode code = &r->stack[r->it_stack++];
	code->tc = string_tc;
	strcpy(code->str_val, value);
}

static int peekbytecode_(hscriptlet *r, int *level)
{
	if (r->it == r->s->it_codes)
		return empty_tc;

	bytecode code = &r->s->codes[r->it];
	r->level = code->level;
	return code->tc;
}

static void skipbytecode_(hscriptlet *r)
{
	if (r->it == r->s->it_codes)
		return;

	r->it++;
}

static int nextbytecode_(hscriptlet *r, bytecode *value, int *nbr_params, int *level)
{
	if (r->it == r->s->it_codes)
		return empty_tc;

	bytecode code = &r->s->codes[r->it++];
	int bc = code->tc;
	*nbr_params = code->nbr_params;
	*level = code->level;

	if (bc == int_tc)
		*value = code;
	else if (bc == real_tc)
		*value = code;
	else if (bc == string_tc)
		*value = code;
	else if (bc == stack_tc)
		bc = pop_stack(r, value);

	return bc;
}

int scriptlet_run(hscriptlet *r)
{
	r->it_stack = 0;
	r->it_syms = r->it_syms_save;
	r->level = 0;
	r->it = 0;

	double result = 1.0;
	int bc, nbr_params = 0, dummy = 0;
	bytecode value = NULL;
	bytecode val1 = NULL;
	bytecode val2 = NULL;
	int type1 = 0, type2 = 0;
	int done = 0;

	while (!done && (bc = nextbytecode_(r, &value, &nbr_params, &r->level)) != 0)
	{
		DEBUG printf("run level=%d, bc=%d nbr_params=%d\n", r->level, bc, nbr_params);

		if (nbr_params)
		{
			nextbytecode_(r, &val1, &dummy, &dummy);
			val1 = substitute(r, val1);
			type1 = val1->tc;
			nbr_params--;
		}

		if (nbr_params)
		{
			nextbytecode_(r, &val2, &dummy, &dummy);
			val2 = substitute(r, val2);
			type2 = val2->tc;
			nbr_params--;
		}

		switch(bc)
		{
			case empty_tc:
			{
				break;
			}
			case multiply_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, result=v1*v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "*", (double)v2, result);
				}
				else if (type1 == real_tc)
				{
					double v1 = val1->real_val;
					double v2 = (double)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_real(r, result=v1*v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "*", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, "*", val2->str_val, result);
				}

				break;
			}

			case power_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, result=(long long)pow((double)v1,(double)v2));
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "*", (double)v2, result);
				}
				else if (type1 == real_tc)
				{
					double v1 = val1->real_val;
					double v2 = (double)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_real(r, result=pow(v1,v2));
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "*", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, "*", val2->str_val, result);
				}

				break;
			}

			case divide_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "/", (double)v2, result);
					if (v2 == 0) return result = 0.0;
					push_stack_int(r, result=v1/v2);
				}
				else if (type1 == real_tc)
				{
					double v1 = val1->real_val;
					double v2 = (double)(type2==int_tc?val2->int_val:val2->real_val);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "/", (double)v2, result);
					if (v2 == 0.0) return 0.0;
					push_stack_real(r, result=v1/v2);
				}
				else
				{
					return result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, "/", val2->str_val, result);
				}

				break;
			}

			case modulo_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, result=v1%v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "%", (double)v2, result);
				}
				else if (type1 == real_tc)
				{
					long long v1 = (long long)val1->real_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, result=v1%v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "%", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, "%", val2->str_val, result);
				}

				break;
			}

			case assign_tc:
			{
				if (type2 == int_tc)
				{
					long long v2 = (long long)val2->int_val;
					sb_string_set(r->symtab, val1->str_val, (void*)val2);
					push_stack_int(r, result=v2);
					DEBUG printf("run %s %s '%g'\n", val1->str_val, "=", (double)v2);
				}
				else if (type2 == real_tc)
				{
					double v2 = val2->real_val;
					sb_string_set(r->symtab, val1->str_val, (void*)val2);
					push_stack_real(r, result=v2);
					DEBUG printf("run %s %s '%g'\n", val1->str_val, "=", (double)v2);
				}
				else if (type2 == string_tc)
				{
					const char *v2 = val2->str_val;
					void *v3 = NULL;

					if (sb_string_get(r->symtab, v2, &v3))
						sb_string_set(r->symtab, val1->str_val, v3);
					else
						sb_string_set(r->symtab, val1->str_val, (void*)val2);
					push_stack_string(r, v2); result=0.0;
					DEBUG printf("run %s %s %s\n", val1->str_val, "=", v2);
				}
				else
				{
					push_stack_real(r, result=0.0);
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, "+", val2->str_val, result);
				}

				break;
			}

			case add_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, result=v1+v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "+", (double)v2, result);
				}
				else if (type1 == real_tc)
				{
					double v1 = val1->real_val;
					double v2 = (type2==int_tc?val2->int_val:val2->real_val);
					push_stack_real(r, result=v1+v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "+", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, "+", val2->str_val, result);
				}

				break;
			}

			case subtract_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, result=v1-v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "-", (double)v2, result);
				}
				else if (type1 == real_tc)
				{
					double v1 = val1->real_val;
					double v2 = (type2==int_tc?val2->int_val:val2->real_val);
					push_stack_real(r, result=v1-v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "-", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, "-", val2->str_val, result);
				}

				break;
			}

			case eq_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, result=v1==v2?1:0);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "==", (double)v2, result);
				}
				else if (type1 == real_tc)
				{
					double v1 = val1->real_val;
					double v2 = (type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, result=v1==v2?1:0);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "==", (double)v2, result);
				}
				else
				{
					const char *v1 = val1->str_val;
					const char *v2 = val2->str_val;
					push_stack_int(r, !strcmp(v1,v2));
					DEBUG printf("run '%s' %s '%s' = %g\n", val1->str_val, "==", val2->str_val, result);
				}

				break;
			}

			case neq_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, result=v1!=v2?1:0);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "!=", (double)v2, result);
				}
				else if (type1 == real_tc)
				{
					double v1 = val1->real_val;
					double v2 = (type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, result=v1!=v2?1:0);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "!=", (double)v2, result);
				}
				else
				{
					const char *v1 = val1->str_val;
					const char *v2 = val2->str_val;
					push_stack_int(r, strcmp(v1,v2));
					DEBUG printf("run '%s' %s '%s' = %g\n", val1->str_val, "!=", val2->str_val, result);
				}

				break;
			}

			case if_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					result = (double)v1;
					DEBUG printf("run '%g' %s = %g\n", (double)v1, "if", result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s = %g\n", val1->str_val, "if", result);
				}

				if (!result)
				{
					int skip = 0, tmp_level = 0, tmp_code;

					while ((tmp_code = peekbytecode_(r, &tmp_level)) != 0)
					{
						if (tmp_level <= r->level)
							break;

						skipbytecode_(r);
						skip++;
					}
				}

				break;
			}

			case else_tc:
			{
				if (result)
				{
					int skip = 0, tmp_level = 0, tmp_code;

					while ((tmp_code = peekbytecode_(r, &tmp_level)) != 0)
					{
						if (tmp_level <= r->level)
							break;

						skipbytecode_(r);
						skip++;
					}
				}

				DEBUG printf("runstr %s = %g\n", "else", result);
				break;
			}

			case fi_tc:
			{
				DEBUG printf("runstr %s = %g\n", "fi", result);
				break;
			}

			case not_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					result = (double)!v1;
					DEBUG printf("run '%g' %s = %g\n", (double)v1, "!", result);
					if (!v1) done = 1;
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s = %g\n", val1->str_val, "!", result);
				}

				break;
			}

			case and_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, v1&&v2); result=(double)v1&&v2;
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "&&", (double)v2, result);
				}
				else if (type1 == real_tc)
				{
					double v1 = val1->real_val;
					double v2 = (type2==int_tc?val2->int_val:val2->real_val);
					push_stack_real(r, result=v1&&v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "&&", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, "&&", val2->str_val, result);
				}

				break;
			}

			case or_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, v1||v2); result=(double)(v1||v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "||", (double)v2, result);
				}
				else if (type1 == real_tc)
				{
					double v1 = val1->real_val;
					double v2 = (type2==int_tc?val2->int_val:val2->real_val);
					push_stack_real(r, result=v1||v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "||", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, "||", val2->str_val, result);
				}

				break;
			}

			case xor_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, (v1&&!v2) || (v2&&!v1)); result=(double)((v1&&!v2) || (v2&&!v1));
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "^^", (double)v2, result);
				}
				else if (type1 == real_tc)
				{
					double v1 = val1->real_val;
					double v2 = (type2==int_tc?val2->int_val:val2->real_val);
					push_stack_real(r, result=(v1&&!v2) || (v2&&!v1));
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "^^", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, "^^", val2->str_val, result);
				}

				break;
			}

			case bit_negate_tc:
			{
				if (type1 == int_tc)
				{
					unsigned long long v1 = val1->int_val;
					push_stack_int(r, ~v1); result=(double)~v1;
					DEBUG printf("run %s '%g' = %g\n", "~", (double)v1, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr %s '%s' = %g\n", "~", val1->str_val, result);
				}

				break;
			}

			case bit_and_tc:
			{
				if (type1 == int_tc)
				{
					unsigned long long v1 = val1->int_val;
					unsigned long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, v1&v2); result=(double)(v1&v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "&", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, "&", val2->str_val, result);
				}

				break;
			}

			case bit_or_tc:
			{
				if (type1 == int_tc)
				{
					unsigned long long v1 = val1->int_val;
					unsigned long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, v1|v2); result=(double)(v1|v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "|", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, "|", val2->str_val, result);
				}

				break;
			}

			case bit_xor_tc:
			{
				if (type1 == int_tc)
				{
					unsigned long long v1 = val1->int_val;
					unsigned long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, v1^v2); result=(double)(v1^v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "^", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, "^", val2->str_val, result);
				}

				break;
			}

			case lt_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, v1<v2); result=(double)(v1<v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "<", (double)v2, result);
				}
				else if (type1 == real_tc)
				{
					double v1 = val1->real_val;
					double v2 = (type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, v1<v2); result=(double)(v1<v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "<", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, "<", val2->str_val, result);
				}

				break;
			}

			case leq_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, v1<=v2); result=(double)(v1<=v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "<=", (double)v2, result);
				}
				else if (type1 == real_tc)
				{
					double v1 = val1->real_val;
					double v2 = (type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, v1<=v2); result=(double)(v1<=v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "<=", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, "<", val2->str_val, result);
				}

				break;
			}

			case gt_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, v1>v2); result=(double)(v1>v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, ">", (double)v2, result);
				}
				else if (type1 == real_tc)
				{
					double v1 = val1->real_val;
					double v2 = (type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, v1>v2); result=(double)(v1>v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, ">", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, ">", val2->str_val, result);
				}

				break;
			}

			case geq_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, v1>=v2); result=(double)(v1>=v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, ">=", (double)v2, result);
				}
				else if (type1 == real_tc)
				{
					double v1 = val1->real_val;
					double v2 = (type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, v1>=v2); result=(double)(v1>=v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, ">=", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, ">=", val2->str_val, result);
				}

				break;
			}

			case shift_left_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, v1<<v2); result=(double)(v1<<v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, "<<", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, "<<", val2->str_val, result);
				}

				break;
			}

			case shift_right_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, v1>>v2); result=(double)(v1>>v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, ">>", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, ">>", val2->str_val, result);
				}

				break;
			}

			case logical_shift_right_tc:
			{
				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
					push_stack_int(r, ((unsigned long long)v1)>>v2); result=(double)(((unsigned long long)v1)>>v2);
					DEBUG printf("run '%g' %s '%g' = %g\n", (double)v1, ">>>", (double)v2, result);
				}
				else
				{
					result = 0.0;
					DEBUG printf("runstr '%s' %s '%s' = %g\n", val1->str_val, ">>>", val2->str_val, result);
				}

				break;
			}

			case func_is_int_tc:
			{
				push_stack_int(r, (type1==int_tc?1:0)); result=(double)((type1==int_tc?1:0));
				DEBUG printf("run %s '%lld' = %g\n", "int", (long long)val1->int_val, result);
				break;
			}

			case func_is_real_tc:
			{
				push_stack_int(r, (type1==real_tc?1:0)); result=(double)((type1==real_tc?1:0));
				DEBUG printf("run %s '%g' = %g\n", "real", val1->real_val, result);
				break;
			}

			case func_is_nan_tc:
			{
				push_stack_int(r, (type1==real_tc && isnan(val1->real_val))); result=(double)((type1==real_tc && isnan(val1->real_val)));
				DEBUG printf("run %s '%d' = %g\n", "nan", type1, result);
				break;
			}

			case func_is_string_tc:
			{
				push_stack_int(r, (type1==string_tc?1:0)); result=(double)((type1==string_tc?1:0));
				DEBUG printf("run %s '%s' = %g\n", "string", val1->str_val, result);
				break;
			}

			case func_size_tc:
			{
				push_stack_int(r, (type1==string_tc?strlen(val1->str_val):0)); result=(double)(type1==string_tc?strlen(val1->str_val):0);
				DEBUG printf("run %s '%s' = %g\n", "size", val1->str_val, result);
				break;
			}

			case func_fold_case_tc:
			{
				const char *v1 = val1->str_val;
				const char *rv = v1;
				push_stack_string(r, rv); result = 1.0;
				DEBUG printf("run %s '%s' = '%s'\n", "fold-case", val1->str_val, rv);
				break;
			}

			case func_jday_tc:
			{
				const char *v1 = val1->str_val;
				int dd = 0, mm = 0, yy = 0;
				sscanf(v1, "%d/%d/%d", &dd, &mm, &yy);
				int jday = juliandate(yy, mm, dd);
				push_stack_int(r, jday); result = jday;
				DEBUG printf("run '%s' %s = %g\n", val1->str_val, " jday ", result);
				break;
			}

			case func_dow_tc:
			{
				const char *v1 = val1->str_val;
				int dd = 0, mm = 0, yy = 0;
				sscanf(v1, "%d/%d/%d", &dd, &mm, &yy);
				int jdate = juliandate(yy, mm, dd);
				int dow = dayofweek(jdate);
				push_stack_int(r, dow); result = dow;
				DEBUG printf("run '%s' %s = %g\n", val1->str_val, " dow ", result);
				break;
			}

			case func_print_tc:
			{
				result = 1.0;

				if (type1 == int_tc)
				{
					long long v1 = val1->int_val;
					printf("%lld\n", v1);
					DEBUG printf("run '%g' %s = %g\n", (double)v1, "print", result);
				}
				else if (type1 == real_tc)
				{
					double v1 = val1->real_val;
					printf("%.*g\n", DBL_DIG, v1);
					DEBUG printf("run '%g' %s = %g\n", (double)v1, "print", result);
				}
				else
				{
					printf("%s\n", val1->str_val);
					DEBUG printf("run '%s' %s = %g\n", val1->str_val, "print", result);
				}

				break;
			}

			case func_eq_tc:
			{
				int one_more = 0;

				do
				{
					if (type1 == int_tc)
					{
						long long v1 = val1->int_val;
						long long v2 = (long long)(type2==int_tc?val2->int_val:val2->real_val);
						int this_result;
						result += this_result = (v1==v2);
						DEBUG printf("run %s %g %g = %g\n", "equals", (double)v1, (double)v2, (double)this_result);
					}
					else if (type1 == real_tc)
					{
						double v1 = val1->real_val;
						double v2 = (type2==int_tc?val2->int_val:val2->real_val);
						int this_result;
						result += this_result = (v1==v2);
						DEBUG printf("run %s %g %g = %g\n", "equals", (double)v1, (double)v2, (double)this_result);
					}
					else if (type1 == string_tc)
					{
						const char *v1 = val1->str_val;
						const char *v2 = val2->str_val;
						int this_result;
						result += this_result = !strcmp(v1, v2);
						DEBUG printf("run %s '%s' '%s' = %g\n", "equals", v1, v2, (double)this_result);
					}

					one_more = nbr_params;

				 	if (one_more)
				 	{
				 		nextbytecode_(r, &val2, &dummy, &dummy);
						val2 = substitute(r, val2);
						type2 = val2->tc;
				 		nbr_params--;
				 	}
				}
				 while (one_more);

				push_stack_int(r, (long long)result);
				break;
			}

			case func_contains_tc:
			{
				int one_more = 0;

				do
				{
					if (type1 == string_tc)
					{
						const char *v1 = val1->str_val;
						const char *v2 = val2->str_val;
						int this_result;
						result += this_result = strstr(v1, v2) != 0;
						DEBUG printf("run %s '%s' '%s' = %g\n", "contains", v1, v2, (double)this_result);
					}

					one_more = nbr_params;

				 	if (one_more)
				 	{
				 		type2 = nextbytecode_(r, &val2, &dummy, &dummy);
						val2 = substitute(r, val2);
						type2 = val2->tc;
				 		nbr_params--;
				 	}
				}
				 while (one_more);

				push_stack_int(r, (long long)result);
				break;
			}

			case func_begins_with_tc:
			{
				int one_more = 0;

				do
				{
					if (type1 == string_tc)
					{
						const char *v1 = val1->str_val;
						const char *v2 = val2->str_val;
						int this_result;
						result += this_result = !strncmp(v1, v2, strlen(v2));
						DEBUG printf("run %s '%s' '%s' = %g\n", "begins-with", v1, v2, (double)this_result);
					}

					one_more = nbr_params;

				 	if (one_more)
				 	{
				 		type2 = nextbytecode_(r, &val2, &dummy, &dummy);
						val2 = substitute(r, val2);
						type2 = val2->tc;
				 		nbr_params--;
				 	}
				}
				 while (one_more);

				push_stack_int(r, (long long)result);
				break;
			}

			case func_ends_with_tc:
			{
				int one_more = 0;

				do
				{
					if (type1 == string_tc)
					{
						const char *v1 = val1->str_val;
						const char *v2 = val2->str_val;
						int this_result;
						result += this_result = ends_with(v1, v2);
						DEBUG printf("run %s '%s' '%s' = %g\n", "ends-with", v1, v2, (double)this_result);
					}

					one_more = nbr_params;

				 	if (one_more)
				 	{
				 		type2 = nextbytecode_(r, &val2, &dummy, &dummy);
						val2 = substitute(r, val2);
						type2 = val2->tc;
				 		nbr_params--;
				 	}
				}
				 while (one_more);

				push_stack_int(r, (long long)result);
				break;
			}

			default:

				printf("Execution error: unknown bytecode=%d, %s\n", bc, value->str_val);
				done = 1;
				break;
		};
	}

	return 1;
}

scriptlet *scriptlet_open(const char *text)
{
	scriptlet *s = (scriptlet*)calloc(1, sizeof(struct scriptlet_));
	if (!s) return NULL;

	if (!compile(s, text))
	{
		free(s);
		return NULL;
	}

	scriptlet_share(s);
	return s;
}

int scriptlet_close(scriptlet *s)
{
	if (!s)
		return 0;

	scriptlet_unshare(s);
	return 1;
}

