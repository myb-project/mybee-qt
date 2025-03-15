#!/usr/bin/env bash

# Application settings
APP_NAME="mybee-qt"
TS_DIR="src/translations"

# Function to find Qt tools path
find_qt_path() {
    # Try to use qtpaths if available
    if command -v qtpaths6 &> /dev/null; then
        QT_PATH=$(qtpaths6 --binaries-dir)
        return 0
    elif command -v qtpaths &> /dev/null; then
        QT_PATH=$(qtpaths --binaries-dir)
        return 0
    fi

    # Check common paths based on OS
    case "$(uname -s)" in
        Linux*)
            for path in /usr/lib/qt6/bin /usr/lib64/qt6/bin /usr/share/qt6/bin; do
                if [ -d "$path" ]; then
                    QT_PATH=$path
                    return 0
                fi
            done
            ;;
        FreeBSD*)
            if [ -d "/usr/local/lib/qt6/bin" ]; then
                QT_PATH="/usr/local/lib/qt6/bin"
                return 0
            fi
            ;;
    esac

    # If Qt path not found, check if lupdate and lrelease are in PATH
    if command -v lupdate &> /dev/null && command -v lrelease &> /dev/null; then
        QT_PATH=""  # Empty path means use tools directly from PATH
        return 0
    fi

    return 1  # Qt path not found
}

# Function to process a language
process_language() {
    local lang_input=$1
    local ts_path
    
    # Check if input is already in language_COUNTRY format
    if [[ "$lang_input" == *"_"* ]]; then
        # Already in locale format, use directly
        ts_path="${TS_DIR}/${APP_NAME}-${lang_input}.ts"
    else
        # Convert simple language code to locale format
        local locale
        case "$lang_input" in
            ru) locale="ru_RU" ;;
            fr) locale="fr_FR" ;;
            it) locale="it_IT" ;;
            # Add more as needed
            *) locale="${lang_input}_${lang_input^^}" ;;  # Default to LANG_LANG format
        esac
        ts_path="${TS_DIR}/${APP_NAME}-${locale}.ts"
    fi
    
    echo "Processing language: $lang_input (${ts_path})"
    
    # Create translation directory if it doesn't exist
    mkdir -p "$TS_DIR"
    
    # Run lupdate to create/update translation file
    if [ -z "$QT_PATH" ]; then
        lupdate "src/src.pro" -ts "$ts_path"
    else
        "$QT_PATH/lupdate" "src/src.pro" -ts "$ts_path"
    fi
    
    if [ -z "$QT_PATH" ]; then
        lrelease "$ts_path"
    else
        "$QT_PATH/lrelease" "$ts_path"
    fi
}

# Main script starts here

# Find Qt path
if ! find_qt_path; then
    echo "Error: Could not find Qt tools path. Please make sure Qt is installed."
    exit 1
fi

if [ -n "$QT_PATH" ]; then
    echo "Using Qt tools from: $QT_PATH"
else
    echo "Using Qt tools from PATH"
fi

# Check if at least one language code is provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 <language_code> [language_code...]"
    echo "Example: $0 ru fr de"
    echo "Note: You can use either language codes (e.g., 'ru') or locale formats (e.g., 'ru_RU')"
    exit 1
fi

# Process each language provided as argument
for lang in "$@"; do
    process_language "$lang"
done

echo "Translation update completed successfully!"
