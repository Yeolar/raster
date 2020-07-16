# git@branch or git@commit or tarball@root
DEPS=(
https://github.com/Yeolar/accelerator@v2
)

mkdir -p _deps && cd _deps
if [ ! -f dep-builder.py ]; then
    curl -L https://github.com/Yeolar/dep-builder/tarball/master | tar xz --strip 2 -C .
fi

python dep-builder.py - "${DEPS[@]}"
