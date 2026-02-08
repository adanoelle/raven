# Doxygen API reference

# Generate HTML and XML from docstrings
build:
    doxygen Doxyfile

# Serve generated HTML locally
serve: build
    @echo "Serving API docs at http://localhost:8080"
    npx -y serve ../build-docs/api -l 8080 --no-clipboard

# Remove generated output
clean:
    rm -rf ../build-docs/api ../build-docs/doxygen-xml
