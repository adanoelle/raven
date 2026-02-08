# Doxygen API reference

# Generate HTML and XML from docstrings
build:
    doxygen Doxyfile

# Serve generated HTML locally
serve: build
    @echo "Serving API docs at http://localhost:8080"
    python3 -m http.server 8080 -d ../build-docs/api

# Remove generated output
clean:
    rm -rf ../build-docs/api ../build-docs/doxygen-xml
