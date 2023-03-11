/*
 * Copyright © 2023 Github Lzhiyong
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <tree_sitter/api.h>

// language c
extern TSLanguage *tree_sitter_c();

//... other languages

typedef void (*Function)(const char*, const char*);


void indent(const int count) {
    for (int i=0; i < count * 2; ++i) {
        printf("%c", 0x20); // whitespace ascii 0x20
    }
}

void measure_time(Function func, const char *p1, const char *p2) {
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
    
    // measure parser time
    func(p1, p2);
    
    clock_gettime(CLOCK_REALTIME, &end);
    double time_spent = ((end.tv_sec - start.tv_sec) +
                        (end.tv_nsec - start.tv_nsec) / 1000000000.0);
    
    if((int)time_spent >= 1)
        printf("parser cost time: %.2lfs\n", time_spent);
    else
        printf("parser cost time: %.2lfms\n", time_spent * 1000);
}

// traverse the syntax tree
void traverse(TSNode root, TSTreeCursor *cursor) {

    bool visited = false;
    // indent count
    int count = 0;

    for(;;) {
        if(visited) {
            if(ts_tree_cursor_goto_next_sibling(cursor)) {
                visited = false;
            } else if(ts_tree_cursor_goto_parent(cursor)) {
                visited = true;
                count--;
            } else {
                break;
            }
        } else {
            TSNode node = ts_tree_cursor_current_node(cursor);
            if(ts_node_is_named(node)) {
                indent(count);
                const char *field = ts_tree_cursor_current_field_name(cursor);
                if(field != NULL) {
                    printf("%s: ", field);
                }
                
                const char *type = ts_node_type(node);
                // int start = ts_node_start_byte(node);
                // int end = ts_node_end_byte(node);
                
                TSPoint startPoint = ts_node_start_point(node);
                TSPoint endPoint = ts_node_end_point(node);
                
                printf("%s [%u, %u] - [%u, %u]\n", type, startPoint.row, startPoint.column, endPoint.row, endPoint.column);
            }
            
            if(ts_tree_cursor_goto_first_child(cursor)) {
                visited = false;
                count++;
            } else {
                visited = true;
            }
        }
    }
}

// query the syntax tree
void queries(
    const TSNode root,
    const TSLanguage *language,
    const char *source,
    const char *expression
) {
    //     
    uint32_t length = strlen(expression);
    uint32_t error_offset;
    TSQueryError error_type;
    
    TSQuery *query = ts_query_new(language, expression, length, &error_offset, &error_type);
    TSQueryCursor *cursor = ts_query_cursor_new();
    
    // execute query
    ts_query_cursor_exec(cursor, query, root);
    
    TSQueryMatch match;
    while(ts_query_cursor_next_match(cursor, &match)) {
        for(int i=0; i < match.capture_count; ++i) {
            TSNode node = match.captures[i].node;
            uint32_t length = 0;
            const char *name = ts_query_capture_name_for_id(query, match.captures[i].index, &length);
            
            int start = ts_node_start_byte(node);
            int end = ts_node_end_byte(node);
            
            char text[end - start + 1];
            memset(text, '\0', sizeof(text));
            strncpy(text, source + start, end - start);
            
            TSPoint startPoint = ts_node_start_point(node);
            TSPoint endPoint = ts_node_end_point(node);
            
            printf("%s -> %s ", text, name);
            printf("[%u, %u] - [%u, %u]\n", startPoint.row, startPoint.column, endPoint.row, endPoint.column);
        }
    }

    printf("capture count: %d\n", ts_query_capture_count(query));
    printf("pattern count: %d\n", ts_query_pattern_count(query));
    printf("string count: %d\n", ts_query_string_count(query));
    
    ts_query_cursor_delete(cursor);
    ts_query_delete(query);
}

// string buffer
typedef struct {
    const char *text;
    int length;
}StringBuffer;

const char* callback(void *payload, uint32_t byte_index, TSPoint point, uint32_t *bytes_read) {
    StringBuffer *buffer = (StringBuffer*)payload;
  
    if (byte_index >= buffer->length) {
        *bytes_read = 0;
        return NULL;
    } else {
        *bytes_read = buffer->length - byte_index;
        return buffer->text + byte_index;
    }
}

// test parser parse
void test_parser_parse(const char *source, const char *expression) {
    // create parser
    TSParser *parser = ts_parser_new();
    TSLanguage *language = tree_sitter_c();
    ts_parser_set_language(parser, language);
    
    StringBuffer buffer = {source, strlen(source)};
    
    TSInput input = {&buffer, callback, TSInputEncodingUTF8};
    
    // syntax tree
    TSTree *tree = ts_parser_parse(parser, NULL, input);
    
    // root node
    TSNode root = ts_tree_root_node(tree);
    TSTreeCursor cursor = ts_tree_cursor_new(root);
    
    traverse(root, &cursor);
    queries(root, language, source, expression);
    
    ts_tree_cursor_delete(&cursor);
    ts_tree_delete(tree);
    ts_parser_delete(parser);
}

// test parse string
void test_parse_string(const char *source, const char *expression) {
    // create parser
    TSParser *parser = ts_parser_new();
    TSLanguage *language = tree_sitter_c();
    ts_parser_set_language(parser, language);
    
    // syntax tree
    TSTree *tree = ts_parser_parse_string(
        parser,
        NULL,
        source,
        strlen(source)
    );
    
    // root node
    TSNode root = ts_tree_root_node(tree);
    TSTreeCursor cursor = ts_tree_cursor_new(root);
    
    traverse(root, &cursor);
    queries(root, language, source, expression);
    
    ts_tree_cursor_delete(&cursor);
    ts_tree_delete(tree);
    ts_parser_delete(parser);
}


int main (int argc, char *argv[]) {

    const char *source = "#include <stdio.h>\n\nint main() {\n\tprintf(\"hello world\\n\");\n\treturn 0;\n}\n";
    const char *expression = "\"return\" @keyword.return";
    
    // test parse string
    printf("================= parse string ===================\n");
    measure_time(test_parse_string, source, expression);
    
    printf("\n");
    
    // test parser parse
    printf("================= parser parse ===================\n");
    measure_time(test_parser_parse, source, expression);
    
    return 0;
}
