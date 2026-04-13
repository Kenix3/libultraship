import sys
import os
from cryptography.hazmat.primitives import serialization

def generate_multi_key_header(output_path, pem_files):
    key_variable_names = []

    with open(output_path, 'w') as out_f:
        out_f.write("#pragma once\n\n")
        out_f.write("#include <string_view>\n")
        out_f.write("#include <array>\n")
        out_f.write("#include <cstddef>\n\n")
        out_f.write("// Auto-generated file containing public keys\n\n")

        out_f.write("struct DefaultKey {\n")
        out_f.write("    std::string_view name;\n")
        out_f.write("    std::string_view data;\n")
        out_f.write("};\n\n")

        for pem_path in pem_files:
            base_name = os.path.splitext(os.path.basename(pem_path))[0]

            with open(pem_path, "rb") as key_f:
                key_data = key_f.read()

            try:
                public_key_obj = serialization.load_pem_public_key(key_data)
            except ValueError:
                private_key = serialization.load_pem_private_key(
                    key_data,
                    password=None
                )
                public_key_obj = private_key.public_key()

            raw_public_key = public_key_obj.public_bytes(
                encoding=serialization.Encoding.Raw,
                format=serialization.PublicFormat.Raw
            )
            public_key_hex = raw_public_key.hex()

            var_name = f"{base_name}_data"
            out_f.write(f'inline constexpr std::string_view {var_name} = "{public_key_hex}";\n')
            key_variable_names.append((base_name, var_name))

        num_keys = len(key_variable_names)

        out_f.write("\n// Table of all public keys\n")
        out_f.write(f"inline constexpr std::array<DefaultKey, {num_keys}> AllDefaultKeys = {{\n")
        for string_name, var_name in key_variable_names:
            out_f.write(f'    DefaultKey{{ "{string_name}", {var_name} }},\n')
        out_f.write("};\n\n")

        out_f.write(f"inline constexpr std::size_t AllDefaultKeysSize = {num_keys};\n")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python generate_keys_header.py <output_header_path> <key1.pem> [key2.pem ...]")
        sys.exit(1)

    output_header = sys.argv[1]
    input_pems = sys.argv[2:]
    generate_multi_key_header(output_header, input_pems)