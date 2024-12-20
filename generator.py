import sys
import re
import itertools

cpp_template = """
#include "FluidSimulator.h"
#include <cstdlib>
#include <unordered_map>
#include <variant>
#include <string>
#include <iostream>
#include <vector>


void replaceBrackets(std::string& input) {
    for (char& c : input) {
        if (c == '(') c = '<';
        else if (c == ')') c = '>';
    }
}

using FluidSimulatorVariant = std::variant<{{types}}>;

int main(int argc, char* argv[]) {
    std::unordered_map<std::string, int> params = { {{params}} };
    std::vector<FluidSimulatorVariant> arr = { {{types_vec}} };

    std::string p_type, v_type, v_flow_type, size;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("--p-type=") == 0) p_type = arg.substr(9);
        else if (arg.find("--v-type=") == 0) v_type = arg.substr(9);
        else if (arg.find("--v-flow-type=") == 0) v_flow_type = arg.substr(14);
        else if (arg.find("--size=") == 0) size = arg.substr(7);
    }

    std::string args_str = p_type + " " + v_type + " " + v_flow_type + ", " + size;
    replaceBrackets(args_str);

    auto it = params.find(args_str);
    if (it == params.end()) {
        std::cout << "No suitable params for " << args_str << std::endl;
        return 1;
    }

    size_t T = 2500;
    size_t tick_for_save = 50;
    std::string input_file = "../input.json";
    std::visit([&](auto& simulator) { 
        simulator.runSimulation(T, tick_for_save, input_file); 
    }, arr[it->second]);
    return 0;
}
"""

def normalize_fast_fixed(input_string):
    """Replace FAST_FIXED(N, M) -> FAST_FIXED<N, M>"""
    return re.sub(r'FAST_FIXED\((\d+),\s*(\d+)\)', r'FAST_FIXED<\1,\2>', input_string)

def parse_types(input_string):
    """Parse and normalize types."""
    input_string = normalize_fast_fixed(input_string)
    input_string = re.sub(r'FIXED\((\d+),\s*(\d+)\)', r'FIXED<\1,\2>', input_string)
    type_strings = re.findall(r'FAST_FIXED<\d+,\d+>|FIXED<\d+,\d+>|double|float', input_string)
    return type_strings

def parse_string(input_string):
    pattern = r"S\((\d+),(\d+)\)"
    matches = re.findall(pattern, input_string)
    return [[int(x), int(y)] for x, y in matches]


def create_combinations(types, sizes):
    """Create all possible combinations of types and sizes."""
    return [
        f"{', '.join(p)}, {size[0]}, {size[1]}"
        for p in itertools.product(types, repeat=3)
        for size in sizes
    ]

def generate_code(types, sizes):
    """Generate variant, vector, and params."""
    type_combinations = create_combinations(types, sizes)
    types_variant = ", ".join(f"FluidSimulator<{t}>" for t in type_combinations)
    types_vec = ", ".join(f"FluidSimulator<{t}>()" for t in type_combinations)
    params_map = ", ".join(f'{{"{t}", {i}}}' for i, t in enumerate(type_combinations))
    return types_variant, types_vec, params_map

if len(sys.argv) < 3:
    print("Usage: python generate_code.py <TYPES> <SIZES>")
    sys.exit(1)

types_value = sys.argv[1]
sizes_value = sys.argv[2]

parsed_types = parse_types(types_value)
sizes = parse_string(sizes_value)

variant, vec, params = generate_code(parsed_types, sizes)
rendered_code = cpp_template.replace("{{types}}", variant).replace("{{types_vec}}", vec).replace("{{params}}", params)

with open("main.cpp", "w") as cpp_file:
    cpp_file.write(rendered_code)

print("Generated main.cpp")