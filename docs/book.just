# mdBook documentation

# Build the book
build:
    mdbook build book

# Serve with live reload
serve:
    mdbook serve book --open

# Remove generated output
clean:
    rm -rf ../build-docs/book
