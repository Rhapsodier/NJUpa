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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// 缓冲区大小
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // 比buf稍大
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

// 当前写入位置
static int buf_pos = 0;

// 生成随机空格
static void gen_rand_space() {
    if (rand() % 4 == 0) { // 25%概率插入空格
        if (buf_pos < sizeof(buf) - 1) {
            buf[buf_pos++] = ' ';
        }
    }
}

// 生成随机数字
static void gen_rand_num() {
    gen_rand_space();
    
    // 生成1-9的随机数字，避免前导0
    if (buf_pos < sizeof(buf) - 1) {
        buf[buf_pos++] = '1' + rand() % 9;
    }
    
    
    gen_rand_space();
}

// 生成随机运算符
static void gen_rand_op() {
    const char ops[] = "+-*/";
    char op = ops[rand() % 4];
    
    if (buf_pos < sizeof(buf) - 1) {
        buf[buf_pos++] = op;
    }
}

// 递归生成随机表达式
static void gen_rand_expr_rec(int depth) {
    if (depth > 10 || buf_pos >= sizeof(buf) - 10) {
        gen_rand_num();
        return;
    }

    switch (rand() % 3) {
        case 0: // 数字
            gen_rand_num();
            break;
        case 1: { // 括号表达式
            if (buf_pos < sizeof(buf) - 2) {
                buf[buf_pos++] = '(';
                gen_rand_space();
                gen_rand_expr_rec(depth + 1);
                gen_rand_space();
                if (buf_pos < sizeof(buf) - 1) {
                    buf[buf_pos++] = ')';
                }
            }
            break;
        }
        case 2: { // 二元运算
            gen_rand_expr_rec(depth + 1);
            gen_rand_space();
            gen_rand_op();
            gen_rand_space();
            gen_rand_expr_rec(depth + 1);
            break;
        }
    }
}

// 生成随机表达式的主函数
static void gen_rand_expr() {
    buf_pos = 0;
    buf[0] = '\0';
    gen_rand_expr_rec(0);
    buf[buf_pos] = '\0'; // 确保字符串终止
}

int main(int argc, char *argv[]) {
    int seed = time(0);
    srand(seed);
    int loop = 1;
    if (argc > 1) {
        sscanf(argv[1], "%d", &loop);
    }
    int i;
    for (i = 0; i < loop; i++) {
        gen_rand_expr();

        sprintf(code_buf, code_format, buf);

        FILE *fp = fopen("/tmp/.code.c", "w");
        assert(fp != NULL);
        fputs(code_buf, fp);
        fclose(fp);

        int ret = system("gcc /tmp/.code.c -o /tmp/.expr 2>/dev/null");
        if (ret != 0) continue;

        fp = popen("/tmp/.expr", "r");
        assert(fp != NULL);

        int result;
        ret = fscanf(fp, "%d", &result);
        pclose(fp);

        // 检查除0错误
        if (strstr(buf, "/ 0") == NULL && strstr(buf, "/0") == NULL) {
            printf("%u %s\n", result, buf);
        }
    }
    return 0;
}