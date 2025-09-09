def _headers_repo_impl(ctx):
    # Pick the first include prefix that contains the requested subdir.
    subdir = ctx.attr.subdir
    libname = ctx.attr.libname if ctx.attr.libname else subdir
    candidates = ctx.attr.paths if ctx.attr.paths and len(ctx.attr.paths) > 0 else [ctx.attr.path]
    chosen = None
    for cand in candidates:
        p = ctx.path(cand).get_child(subdir)
        if p.exists:
            chosen = ctx.path(cand)
            break
    if chosen == None:
        fail("Could not locate headers subdir '{}' under any of: {}".format(subdir, candidates))

    ctx.symlink(chosen.get_child(subdir), subdir)
    ctx.file(
        "BUILD.bazel",
        """
load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "{libname}",
    hdrs = glob(["{subdir}/**"]),
    includes = ["."],
    visibility = ["//visibility:public"],
)
""".format(libname = libname, subdir = subdir),
    )

headers_repo = repository_rule(
    implementation = _headers_repo_impl,
    attrs = {
        # Back-compat single path; used if 'paths' not provided
        "path": attr.string(default = "/opt/homebrew/include"),
        # Preferred: probe these include prefixes in order. Covers macOS and Linux.
        "paths": attr.string_list(default = [
            "/opt/homebrew/include",  # macOS Homebrew (ARM/x86)
            "/usr/include",           # Linux system include
            "/usr/local/include",    # Fallback/local
        ]),
        "subdir": attr.string(mandatory = True),
        "libname": attr.string(),
    },
)

def _couchbase_ext_impl(module_ctx):
    headers_repo(name = "couchbase_cxx_client", subdir = "couchbase", libname = "couchbase")
    headers_repo(name = "fmt_headers", subdir = "fmt", libname = "fmt")
    headers_repo(name = "tao_json_headers", subdir = "tao", libname = "tao_json")

couchbase_ext = module_extension(implementation = _couchbase_ext_impl)


