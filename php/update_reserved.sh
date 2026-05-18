#!/bin/bash

# The single source of truth for reserved words, categorized by PHP documentation
# Taken from https://www.php.net/manual/en/reserved.keywords.php
PHP_RESERVED_KEYWORDS=(
    "abstract" "and" "array" "as" "break" "callable" "case" "catch"
    "class" "clone" "const" "continue" "declare" "default" "die" "do"
    "echo" "else" "elseif" "empty" "enddeclare" "endfor" "endforeach"
    "endif" "endswitch" "endwhile" "eval" "exit" "extends" "final"
    "finally" "fn" "for" "foreach" "function" "global" "goto" "if"
    "implements" "include" "include_once" "instanceof" "insteadof"
    "interface" "isset" "list" "match" "namespace" "new" "or" "parent"
    "print" "private" "protected" "public" "readonly" "require"
    "require_once" "return" "self" "static" "switch" "throw" "trait"
    "try" "unset" "use" "var" "while" "xor" "yield"
)

# Taken from https://www.php.net/manual/en/reserved.other-reserved-words.php
PHP_OTHER_RESERVED_WORDS=(
    "parent" "self" "int" "float" "bool" "string" "true" "false" "null"
    "void" "iterable" "object" "mixed" "never" "array" "callable"
)

# Derived lists:
# RESERVED_WORDS is the union of keywords and soft reserved words.
RESERVED_WORDS=($(echo "${PHP_RESERVED_KEYWORDS[@]}" "${PHP_OTHER_RESERVED_WORDS[@]}" | tr ' ' '\n' | sort -u))

# This is for backwards compatibility. New PHP_RESERVED_KEYWORDS should be added
# here to prevent unnecessary breaking changes, as constants do not need the PB prefix.
# @TODO: allow ALL names for constants except for "class"
VALID_CONSTANT_NAMES=(
    "bool" "false" "float" "int" "iterable" "mixed" "never" "null"
    "object" "parent" "readonly" "self" "string" "true" "void"
)
VALID_CONSTANT_NAMES=($(echo "${VALID_CONSTANT_NAMES[@]}" | tr ' ' '\n' | sort -u))

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

format_columns() {
    local per_line=$1
    local indent=$2
    shift 2
    local words=("$@")
    local num_words=${#words[@]}
    
    # Default to original behavior of quoting the word.
    local item_fmt="${ITEM_FMT:-\"%s\"}"
    if [[ -z "$ITEM_FMT" ]]; then
        item_fmt="\"%s\""
    fi
    
    local max_len=0
    for w in "${words[@]}"; do
        local s
        printf -v s "$item_fmt" "$w"
        if (( ${#s} > max_len )); then max_len=${#s}; fi
    done
    local col_width=$((max_len + 2))
    
    local i=0
    while (( i < num_words )); do
        local line="$indent"
        local j=0
        while (( j < per_line && i < num_words )); do
            local w="${words[i]}"
            local s
            printf -v s "$item_fmt" "$w"
            if (( i < num_words - 1 )); then
                s+=","
            fi
            
            if (( j < per_line - 1 && i < num_words - 1 )); then
                printf -v padded "%-${col_width}s" "$s"
                line+="$padded"
            else
                line+="$s"
            fi
            
            i=$i+1
            j=$j+1
        done
        echo "${line%" "}"
    done
}

update_file() {
    local path="$1"
    local label="$2"
    local new_content="$3"

    local begin="// BEGIN $label"
    local end="// END $label"
    local full_path="$REPO_ROOT/$path"
    if [[ ! -f "$full_path" ]]; then
        echo "Skipping $path (not found)"
        return
    fi
    
    # Extract indentation of the first line of content
    local indent=$(echo "$new_content" | head -n 1 | grep -o '^[[:space:]]*')

    local temp_file=$(mktemp)
    
    # Use perl to replace the block, writing to a temporary file
    BEGIN_MARKER="$begin" END_MARKER="$end" NEW_CONTENT="$new_content" INDENT="$indent" \
    perl -0777 -pe 's{^[ \t]*\Q$ENV{BEGIN_MARKER}\E.*?^[ \t]*\Q$ENV{END_MARKER}\E}{$ENV{INDENT}$ENV{BEGIN_MARKER}\n$ENV{NEW_CONTENT}\n$ENV{INDENT}$ENV{END_MARKER}}sm' "$full_path" > "$temp_file"
    
    # Move the temporary file to the original file's path
    mv "$temp_file" "$full_path"
    
    echo "Updated $path"
}

generate_names_cc() {
    local lines=()
    lines+=("// @see php/update_reserved.sh - DO NOT MODIFY THIS LIST MANUALLY")
    lines+=("const char* const kReservedNames[] = {")
    local col_lines=()
    while IFS= read -r line; do
        col_lines+=("$line")
    done < <(format_columns 5 "    " "${RESERVED_WORDS[@]}")
    for l in "${col_lines[@]}"; do lines+=("$l"); done
    lines[${#lines[@]}-1]+="};"
    lines+=("const int kReservedNamesSize = ${#RESERVED_WORDS[@]};")
    printf "%s\n" "${lines[@]}"
}

generate_names_c() {
    local lines=()
    lines+=("// @see php/update_reserved.sh - DO NOT MODIFY THIS LIST MANUALLY")
    lines+=("const char* const kReservedNames[] = {")
    local decorated=()
    for w in "${RESERVED_WORDS[@]}"; do
        decorated+=("\"$w\"")
    done
    decorated+=("NULL")
    local col_lines=()
    while IFS= read -r line; do
        col_lines+=("$line")
    done < <(ITEM_FMT="%s" format_columns 5 "    " "${decorated[@]}")
    for l in "${col_lines[@]}"; do lines+=("$l"); done
    lines[${#lines[@]}-1]+="};"
    printf "%s\n" "${lines[@]}"
}

generate_php_generator_cc() {
    local lines=()
    lines+=("// @see php/update_reserved.sh - DO NOT MODIFY THIS LIST MANUALLY")
    lines+=("constexpr absl::string_view kValidConstantNames[] = {")
    local col_lines=()
    while IFS= read -r line; do
        col_lines+=("$line")
    done < <(format_columns 6 "    " "${VALID_CONSTANT_NAMES[@]}")
    for l in "${col_lines[@]}"; do lines+=("$l"); done
    lines[${#lines[@]}-1]+="};"
    lines+=("const int kValidConstantNamesSize = ${#VALID_CONSTANT_NAMES[@]};")
    printf "%s\n" "${lines[@]}"
}

generate_proto_messages() {
    local upper=$1
    echo "// @see php/update_reserved.sh - DO NOT MODIFY THIS LIST MANUALLY"
    for w in "${RESERVED_WORDS[@]}"; do
        local name="$w"
        if [[ "$upper" == "true" ]]; then name=$(echo "$w" | tr '[:lower:]' '[:upper:]'); fi
        echo "message $name {}"
    done
}

generate_proto_enums() {
    local upper=$1
    echo "// @see php/update_reserved.sh - DO NOT MODIFY THIS LIST MANUALLY"
    local i=1
    for w in "${RESERVED_WORDS[@]}"; do
        local name="$w"
        if [[ "$upper" == "true" ]]; then name=$(echo "$w" | tr '[:lower:]' '[:upper:]'); fi
        echo "enum $name { ZERO$i = 0; }"
        i=$((i + 1))
    done
}

generate_proto_enum_values() {
    local upper=$1
    echo "  // @see php/update_reserved.sh - DO NOT MODIFY THIS LIST MANUALLY"
    local i=0
    for w in "${RESERVED_WORDS[@]}"; do
        local name="$w"
        if [[ "$upper" == "true" ]]; then name=$(echo "$w" | tr '[:lower:]' '[:upper:]'); fi
        echo "  $name = $i;"
        i=$((i + 1))
    done
}

generate_test_cases() {
    echo "        // @see php/update_reserved.sh - DO NOT MODIFY THIS LIST MANUALLY"
    # Lower messages
    for w in "${RESERVED_WORDS[@]}"; do
        echo "        \$m = new \\Lower\\PB$w();"
    done
    echo ""
    # Upper messages
    for w in "${RESERVED_WORDS[@]}"; do
        local upper=$(echo "$w" | tr '[:lower:]' '[:upper:]')
        echo "        \$m = new \\Upper\\PB$upper();"
    done
    echo ""
    # Lower enums
    for w in "${RESERVED_WORDS[@]}"; do
        echo "        \$m = new \\Lower_enum\\PB$w();"
    done
    echo ""
    # Upper enums
    for w in "${RESERVED_WORDS[@]}"; do
        local upper=$(echo "$w" | tr '[:lower:]' '[:upper:]')
        echo "        \$m = new \\Upper_enum\\PB$upper();"
    done
    echo ""
    # Lower enum values
    for w in "${RESERVED_WORDS[@]}"; do
        local prefix="PB"
        for v in "${VALID_CONSTANT_NAMES[@]}"; do
            if [[ "$w" == "$v" ]]; then prefix=""; break; fi
        done
        echo "        \$m = \\Lower_enum_value\\NotAllowed::${prefix}${w};"
    done
    echo ""
    # Upper enum values
    for w in "${RESERVED_WORDS[@]}"; do
        local prefix="PB"
        for v in "${VALID_CONSTANT_NAMES[@]}"; do
            if [[ "$w" == "$v" ]]; then prefix=""; break; fi
        done
        local upper=$(echo "$w" | tr '[:lower:]' '[:upper:]')
        echo "        \$m = \\Upper_enum_value\\NotAllowed::${prefix}${upper};"
    done
}

generate_gpbutil_reserved_words() {
    echo "        // @see php/update_reserved.sh - DO NOT MODIFY THIS LIST MANUALLY"
    echo "        \$reserved_words = array("
    ITEM_FMT="\"%s\"=>0" format_columns 5 "            " "${RESERVED_WORDS[@]}"
    echo "        );"
}

update_file "src/google/protobuf/compiler/php/names.cc" "RESERVED NAMES" "$(generate_names_cc)"
update_file "php/ext/google/protobuf/names.c" "RESERVED NAMES" "$(generate_names_c)"
update_file "src/google/protobuf/compiler/php/php_generator.cc" "VALID CONSTANT NAMES" "$(generate_php_generator_cc)"
update_file "php/tests/proto/test_reserved_message_lower.proto" "RESERVED NAMES" "$(generate_proto_messages false)"
update_file "php/tests/proto/test_reserved_message_upper.proto" "RESERVED NAMES" "$(generate_proto_messages true)"
update_file "php/tests/proto/test_reserved_enum_lower.proto" "RESERVED NAMES" "$(generate_proto_enums false)"
update_file "php/tests/proto/test_reserved_enum_upper.proto" "RESERVED NAMES" "$(generate_proto_enums true)"
update_file "php/tests/proto/test_reserved_enum_value_lower.proto" "RESERVED NAMES" "$(generate_proto_enum_values false)"
update_file "php/tests/proto/test_reserved_enum_value_upper.proto" "RESERVED NAMES" "$(generate_proto_enum_values true)"
update_file "php/tests/GeneratedClassTest.php" "RESERVED NAMES" "$(generate_test_cases)"
update_file "php/src/Google/Protobuf/Internal/GPBUtil.php" "RESERVED WORDS" "$(generate_gpbutil_reserved_words)"
