/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include "common.h"

enum {
  TK_NOTYPE = 256, TK_EQ, TK_DECIMAL

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},      // spaces
  {"\\+", '+'},           // plus
  {"-", '-'},             // minus
  {"\\*", '*'},           // multiply 
  {"/", '/'},             // divide
  {"\\(", '('},         
  {"\\)", ')'},
  {"==", TK_EQ},          // equal
  {"[0-9]+", TK_DECIMAL}, // +decimal
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;
      
        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;


        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        if(rules[i].token_type == TK_NOTYPE){
          tokens[nr_token].type = rules[i].token_type;
          break;
        }
        
         if(substr_len > sizeof(tokens->str)){
          printf("Token too long at position %d\t (max 32)", position);
          return false;
        }

        switch (rules[i].token_type) {
          case TK_DECIMAL: strncpy(tokens[nr_token].str, substr_start, substr_len);
          tokens[nr_token].str[substr_len] = '\0';
          tokens[nr_token].type = rules[i].token_type;
          nr_token ++;
          break;
          default : 
          tokens[nr_token].type = rules[i].token_type;
          nr_token ++;
      
        
        }
      


        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


 static bool check_parentheses(int p, int q, bool *is_invalid) {
    *is_invalid = false;

    if (tokens[p].type != '(' || tokens[q].type != ')') {
        return false;
    }

    int balance = 0;
    int outer_close_pos = -1; // 记录与首字符 '(' 配对的位置

    for (int i = p; i <= q; ++i) {
        if (tokens[i].type == '(') {
            balance++;
            if (balance == 1 && i != p) { // 首字符 '(' 外的其他 '('
                return false;
            }
        } else if (tokens[i].type == ')') {
            balance--;
            if (balance == 0) {
                outer_close_pos = i;
                break; // 找到外层闭合括号后停止遍历
            }
            if (balance < 0) {
                *is_invalid = true;
                return false;
            }
        }
    }

    // 外层闭合位置必须是 q，且全局平衡
    return (outer_close_pos == q && balance == 0);
} 

static int find_op(int p, int q) {
    int balance = 0;
    int main_op = -1;
    int min_priority = 3;

    const int priority[] = {
        ['+'] = 1, ['-'] = 1,
        ['*'] = 2, ['/'] = 2
    };

    // 从右向左扫描，选择优先级最低的最右侧运算符
    for (int i = q; i >= p; --i) {
        int type = tokens[i].type;
        
        if (type == ')') {
            balance++;
        } else if (type == '(') {
            balance--;
        }
        
        if (balance != 0) continue;

        if (priority[type] > 0) {
            int curr_pri = priority[type];
            if (curr_pri < min_priority) {
                min_priority = curr_pri;
                main_op = i;
            }
        }
    }
    
    return main_op;
}
 



static word_t eval(int p , int q, bool *success){
     bool is_invalid; int op;
  if (p > q) {
      *success = false;
      return 0;
  }
  if (check_parentheses(p, q, &is_invalid)) {
    return eval(p + 1, q - 1, success); // 处理括号内的表达式
  }
  if (is_invalid) {
    *success = false;
    return 0;
  } 
  else if (p == q) {
    if(tokens[p].type == TK_DECIMAL){
      return atoi(tokens[p].str);
    } // 1 token，只能是数字，因此返回数字的值
    *success = false;
    return 0;
  
  }
  else {
    op = find_op(p,q);
    word_t val1 = eval(p, op - 1, success);
    if(!success) return 0;
    word_t val2 = eval(op + 1, q, success);
    if(!success) return 0;

    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/':  if(val2 == 0) {
                    *success = false;
                    return 0;
                }
                return val1 / val2;
            default: assert(0);
    }
  }
  }

  word_t expr (char *e, bool *success){
    if(!make_token(e)){
      *success = false;
      return 0; 
    }
    
 int balance = 0;
for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == '(') {
        balance++;
    } else if (tokens[i].type == ')') {
        balance--;
        if (balance < 0) {  // 中途右括号过多
            *success = false;
            return 0;
        }
    }
}
if (balance != 0) {
    *success = false;
    return 0;
} 
  
  if (nr_token == 0) {
        *success = false;
        return 0;
    }
    
    return eval(0, nr_token-1, success);
    
  }