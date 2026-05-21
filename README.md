## fsistoolx

Utility tool for Financial Integrated Service

## Bundle 

Produce bundle by 

```
docker buildx build \
	-f Dockerfile \
	--target deps-bundle \
	--output type=tar,dest=deps-bundle-stage.tar \
	.

tar -xf deps-bundle-stage.tar deps-bundle.tar.gz
```

## macOS Build

Build the same project components locally on macOS with Homebrew dependencies:

```bash
# First run only, installs the expected Homebrew packages.
scripts/build_macos.sh --install-deps

# Later runs.
scripts/build_macos.sh
```

The script builds:

- `gofunct/build-macos/libgofunct.a`
- `ilfreporter-0.0.1/build-macos/`
- `ilf/build-macos/bin/`
- `ilfx/build-macos/bin/`
- `xsltcli/build-macos/xsltcli`

If dependencies are already installed, skip `--install-deps`. The script expects Homebrew-provided `libxml2`, `libxslt`, `glib`, `gsl`, `xerces-c`, `xqilla`, and `antlr4-cpp-runtime`. It also needs TinyCC/libtcc; on macOS this may need to be installed manually because a Homebrew TinyCC formula is not always available.
