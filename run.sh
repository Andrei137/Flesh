# Set the paths relative to the script location
project_dir="$(dirname "${BASH_SOURCE[0]}")"
cmakelists_file="${project_dir}/CMakeLists.txt"

# Function to check if CMakeLists.txt has been modified
needs_cmake()
{
    if [ "${cmakelists_file}" -nt "${project_dir}" ]; then
        return 0
    else
        return 1
    fi
}

# Check if cmake needs to be rerun
if needs_cmake; then
    echo "Running cmake..."
    cmake .
fi

# Build the project using make
make

# Run the executable
./Flesh