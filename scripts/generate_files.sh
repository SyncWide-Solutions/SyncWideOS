#!/bin/bash

# Create the generated files directory
mkdir -p src/generated

# Start the generated file
cat > src/generated/iso_files_data.c << 'EOF'
// Auto-generated file data from iso_files directory
#include <stdint.h>
#include <stddef.h>

typedef struct {
    const char* filename;
    const char* fat32_name;
    const char* content;
    size_t size;
} iso_file_data_t;

EOF

# Check if iso_files directory exists
if [ ! -d "iso_files" ]; then
    echo "// No iso_files directory found" >> src/generated/iso_files_data.c
    echo "const iso_file_data_t iso_files_data[] = {};" >> src/generated/iso_files_data.c
    echo "const int iso_files_count = 0;" >> src/generated/iso_files_data.c
    echo "Generated 0 files (no iso_files directory)"
    exit 0
fi

# Count files first
file_count=0
for file in iso_files/*; do
    if [ -f "$file" ]; then
        file_count=$((file_count + 1))
    fi
done

if [ $file_count -eq 0 ]; then
    echo "// No files found in iso_files directory" >> src/generated/iso_files_data.c
    echo "const iso_file_data_t iso_files_data[] = {};" >> src/generated/iso_files_data.c
    echo "const int iso_files_count = 0;" >> src/generated/iso_files_data.c
    echo "Generated 0 files (iso_files directory empty)"
    exit 0
fi

# Generate file data
echo "// Generated from iso_files directory" >> src/generated/iso_files_data.c
echo "" >> src/generated/iso_files_data.c

file_index=0
for file in iso_files/*; do
    if [ -f "$file" ]; then
        echo "static const char file_${file_index}_content[] = {" >> src/generated/iso_files_data.c
        
        # Convert file content to C array using od
        od -t x1 -An "$file" | \
        sed 's/^ *//' | \
        sed 's/ *$//' | \
        sed 's/ /\n/g' | \
        grep -v '^$' | \
        sed 's/^/0x/' | \
        sed 's/$/,/' | \
        awk '{
            if (NR % 16 == 1) printf "\n"
            printf "%s ", $0
        }
        END { printf "\n" }' >> src/generated/iso_files_data.c
        
        echo "0x00};" >> src/generated/iso_files_data.c
        echo "" >> src/generated/iso_files_data.c
        
        file_index=$((file_index + 1))
    fi
done

# Generate the array
echo "const iso_file_data_t iso_files_data[] = {" >> src/generated/iso_files_data.c

file_index=0
for file in iso_files/*; do
    if [ -f "$file" ]; then
        filename=$(basename "$file")
        
        # Convert to FAT32 8.3 format
        name_part=$(echo "$filename" | cut -d'.' -f1 | tr '[:lower:]' '[:upper:]')
        ext_part=""
        if [[ "$filename" == *.* ]]; then
            ext_part=$(echo "$filename" | cut -d'.' -f2- | tr '[:lower:]' '[:upper:]')
        fi
        
        # Pad name to 8 chars, extension to 3 chars
        fat32_name=$(printf "%-8s%-3s" "${name_part:0:8}" "${ext_part:0:3}")
        
        file_size=$(wc -c < "$file")
        
        echo "    {" >> src/generated/iso_files_data.c
        echo "        .filename = \"$filename\"," >> src/generated/iso_files_data.c
        echo "        .fat32_name = \"$fat32_name\"," >> src/generated/iso_files_data.c
        echo "        .content = (const char*)file_${file_index}_content," >> src/generated/iso_files_data.c
        echo "        .size = $file_size" >> src/generated/iso_files_data.c
        echo "    }," >> src/generated/iso_files_data.c
        
        file_index=$((file_index + 1))
    fi
done

echo "};" >> src/generated/iso_files_data.c
echo "" >> src/generated/iso_files_data.c
echo "const int iso_files_count = $file_count;" >> src/generated/iso_files_data.c

echo "Generated $file_count files from iso_files directory"
