/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */
#ifndef INCLUDE_git_clone_h__
#define INCLUDE_git_clone_h__

#include "common.h"
#include "types.h"
#include "indexer.h"
#include "checkout.h"
#include "remote.h"
#include "transport.h"


/**
 * @file git2/clone.h
 * @brief Git cloning routines
 * @defgroup git_clone Git cloning routines
 * @ingroup Git
 * @{
 */
GIT_BEGIN_DECL

/**
 * Options for bypassing the git-aware transport on clone. Bypassing
 * it means that instead of a fetch, libgit2 will copy the object
 * database directory instead of figuring out what it needs, which is
 * faster. If possible, it will hardlink the files to save space.
 */
typedef enum {
	/**
	 * Auto-detect (default), libgit2 will bypass the git-aware
	 * transport for local paths, but use a normal fetch for
	 * `file://` urls.
	 */
	GIT_CLONE_LOCAL_AUTO,
	/**
	 * Bypass the git-aware transport even for a `file://` url.
	 */
	GIT_CLONE_LOCAL,
	/**
	 * Do no bypass the git-aware transport
	 */
	GIT_CLONE_NO_LOCAL,
	/**
	 * Bypass the git-aware transport, but do not try to use
	 * hardlinks.
	 */
	GIT_CLONE_LOCAL_NO_LINKS,
} git_clone_local_t;

/**
 * The signature of a function matching git_remote_create, with an additional
 * void* as a callback payload.
 *
 * Callers of git_clone may provide a function matching this signature to override
 * the remote creation and customization process during a clone operation.
 *
 * @param out the resulting remote
 * @param repo the repository in which to create the remote
 * @param name the remote's name
 * @param url the remote's url
 * @param payload an opaque payload
 * @return 0, GIT_EINVALIDSPEC, GIT_EEXISTS or an error code
 */
typedef int (*git_remote_create_cb)(
	git_remote **out,
	git_repository *repo,
	const char *name,
	const char *url,
	void *payload);

/**
 * The signature of a function matchin git_repository_init, with an
 * aditional void * as callback payload.
 *
 * Callers of git_clone my provide a function matching this signature
 * to override the repository creation and customization process
 * during a clone operation.
 *
 * @param out the resulting repository
 * @param path path in which to create the repository
 * @param bare whether the repository is bare. This is the value from the clone options
 * @param payload payload specified by the options
 * @return 0, or a negative value to indicate error
 */
typedef int (*git_repository_create_cb)(
	git_repository **out,
	const char *path,
	int bare,
	void *payload);

/**
 * Clone options structure
 *
 * Use the GIT_CLONE_OPTIONS_INIT to get the default settings, like this:
 *
 *		git_clone_options opts = GIT_CLONE_OPTIONS_INIT;
 */

typedef struct git_clone_options {
	unsigned int version;

	/**
	 * These options are passed to the checkout step. To disable
	 * checkout, set the `checkout_strategy` to
	 * `GIT_CHECKOUT_NONE`. Generally you will want the use
	 * GIT_CHECKOUT_SAFE_CREATE to create all files in the working
	 * directory for the newly cloned repository.
	 */
	git_checkout_options checkout_opts;

	/**
	 * Callbacks to use for reporting fetch progress, and for acquiring
	 * credentials in the event they are needed. This parameter is ignored if
	 * the remote_cb parameter is set; if you provide a remote creation
	 * callback, then you have the opportunity to configure remote callbacks in
	 * provided function.
	 */
	git_remote_callbacks remote_callbacks;

	/**
	 * Set to zero (false) to create a standard repo, or non-zero
	 * for a bare repo
	 */
	int bare;

	/**
	 * Whether to use a fetch or copy the object database.
	 */
	git_clone_local_t local;

	/**
	 * The name of the branch to checkout. NULL means use the
	 * remote's default branch.
	 */
	const char* checkout_branch;

	/**
	 * The identity used when updating the reflog. NULL means to
	 * use the default signature using the config.
	 */
	git_signature *signature;

	/**
	 * A callback used to create the new repository into which to
	 * clone. If NULL, the 'bare' field will be used to determine
	 * whether to create a bare repository.
	 */
	git_repository_create_cb repository_cb;

	/**
	 * An opaque payload to pass to the git_repository creation callback.
	 * This parameter is ignored unless repository_cb is non-NULL.
	 */
	void *repository_cb_payload;

	/**
	 * A callback used to create the git_remote, prior to its being
	 * used to perform the clone operation. See the documentation for
	 * git_remote_create_cb for details. This parameter may be NULL,
	 * indicating that git_clone should provide default behavior.
	 */
	git_remote_create_cb remote_cb;

	/**
	 * An opaque payload to pass to the git_remote creation callback.
	 * This parameter is ignored unless remote_cb is non-NULL.
	 */
	void *remote_cb_payload;
} git_clone_options;

#define GIT_CLONE_OPTIONS_VERSION 1
#define GIT_CLONE_OPTIONS_INIT {GIT_CLONE_OPTIONS_VERSION, {GIT_CHECKOUT_OPTIONS_VERSION, GIT_CHECKOUT_SAFE_CREATE}, GIT_REMOTE_CALLBACKS_INIT}

/**
 * Initializes a `git_clone_options` with default values. Equivalent to
 * creating an instance with GIT_CLONE_OPTIONS_INIT.
 *
 * @param opts The `git_clone_options` struct to initialize
 * @param version Version of struct; pass `GIT_CLONE_OPTIONS_VERSION`
 * @return Zero on success; -1 on failure.
 */
GIT_EXTERN(int) git_clone_init_options(
	git_clone_options *opts,
	unsigned int version);

/**
 * Clone a remote repository.
 *
 * By default this creates its repository and initial remote to match
 * git's defaults. You can use the options in the callback to
 * customize how these are created.
 *
 * @param out pointer that will receive the resulting repository object
 * @param url the remote repository to clone
 * @param local_path local directory to clone to
 * @param options configuration options for the clone.  If NULL, the
 *        function works as though GIT_OPTIONS_INIT were passed.
 * @return 0 on success, any non-zero return value from a callback
 *         function, or a negative value to indicate an error (use
 *         `giterr_last` for a detailed error message)
 */
GIT_EXTERN(int) git_clone(
	git_repository **out,
	const char *url,
	const char *local_path,
	const git_clone_options *options);

/**
 * Clone into a repository
 *
 * After creating the repository and remote and configuring them for
 * paths and callbacks respectively, you can call this function to
 * perform the clone operation and optionally checkout files.
 *
 * @param repo the repository to use
 * @param remote the remote repository to clone from
 * @param co_opts options to use during checkout
 * @param branch the branch to checkout after the clone, pass NULL for the
 *        remote's default branch
 * @param signature The identity used when updating the reflog.
 * @return 0 on success, any non-zero return value from a callback
 *         function, or a negative value to indicate an error (use
 *         `giterr_last` for a detailed error message)
 */
GIT_EXTERN(int) git_clone_into(
	git_repository *repo,
	git_remote *remote,
	const git_checkout_options *co_opts,
	const char *branch,
	const git_signature *signature);

/**
 * Perform a local clone into a repository
 *
 * A "local clone" bypasses any git-aware protocols and simply copies
 * over the object database from the source repository. It is often
 * faster than a git-aware clone, but no verification of the data is
 * performed, and can copy over too much data.
 *
 * @param repo the repository to use
 * @param remote the remote repository to clone from
 * @param co_opts options to use during checkout
 * @param branch the branch to checkout after the clone, pass NULL for the
 *        remote's default branch
 * @param link wether to use hardlinks instead of copying
 * objects. This is only possible if both repositories are on the same
 * filesystem.
 * @param signature the identity used when updating the reflog
 * @return 0 on success, any non-zero return value from a callback
 *         function, or a negative value to indicate an error (use
 *         `giterr_last` for a detailed error message)
 */
GIT_EXTERN(int) git_clone_local_into(
	git_repository *repo,
	git_remote *remote,
	const git_checkout_options *co_opts,
	const char *branch,
	int link,
	const git_signature *signature);

/** @} */
GIT_END_DECL
#endif
