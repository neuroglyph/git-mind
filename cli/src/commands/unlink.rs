// SPDX-License-Identifier: Apache-2.0
// © 2025 J. Kirby Ross / Neuroglyph Collective

//! Unlink command implementation

use crate::commands::GitMindContext;
use crate::error::Result;
use crate::filesystem::FileSystem;
use crate::git::GitOperations;
use crate::link::Link;
use std::path::{Path, PathBuf};

/// Command to remove links between files
pub struct UnlinkCommand<G: GitOperations, F: FileSystem> {
    context: GitMindContext,
    git: G,
    fs: F,
}

impl<G: GitOperations, F: FileSystem> UnlinkCommand<G, F> {
    /// Create a new UnlinkCommand
    pub fn new(working_dir: &Path, git: G, fs: F) -> Result<Self> {
        let context = GitMindContext::new(working_dir)?;
        Ok(Self { context, git, fs })
    }

    /// Remove a specific link between two files
    pub fn execute(&self, source: &str, target: &str, link_type: Option<&str>) -> Result<usize> {
        let links = self.find_links(Some(source), Some(target), link_type)?;
        let count = links.len();

        if count == 0 {
            return Ok(0);
        }

        self.remove_links(&links)?;
        self.commit_removal(source, target, count)?;

        Ok(count)
    }

    /// Remove all links from a source file
    pub fn execute_all_from(&self, source: &str, link_type: Option<&str>) -> Result<usize> {
        let links = self.find_links(Some(source), None, link_type)?;
        let count = links.len();

        if count == 0 {
            return Ok(0);
        }

        self.remove_links(&links)?;
        self.commit_removal_all_from(source, count)?;

        Ok(count)
    }

    /// Remove all links to a target file
    pub fn execute_to(&self, target: &str, link_type: Option<&str>) -> Result<usize> {
        let links = self.find_links(None, Some(target), link_type)?;
        let count = links.len();

        if count == 0 {
            return Ok(0);
        }

        self.remove_links(&links)?;
        self.commit_removal_to(target, count)?;

        Ok(count)
    }

    /// Find links matching the given criteria
    fn find_links(
        &self,
        source: Option<&str>,
        target: Option<&str>,
        link_type: Option<&str>,
    ) -> Result<Vec<(Link, PathBuf)>> {
        let links_dir = self.context.links_dir();
        let mut matching_links = Vec::new();

        // If links directory doesn't exist, there are no links to remove
        if !self.fs.exists(&links_dir) {
            return Ok(matching_links);
        }

        let entries = self.fs.read_dir(&links_dir)?;
        for path in entries {
            if path.extension().and_then(|ext| ext.to_str()) == Some("link") {
                let content = self.fs.read_to_string(&path)?;
                let link = Link::from_canonical_string(&content)?;

                let matches = match (source, target) {
                    (Some(s), Some(t)) => link.source == s && link.target == t,
                    (Some(s), None) => link.source == s,
                    (None, Some(t)) => link.target == t,
                    (None, None) => true,
                };

                let type_matches = link_type.is_none_or(|t| link.link_type == t);

                if matches && type_matches {
                    matching_links.push((link, path));
                }
            }
        }

        Ok(matching_links)
    }

    /// Remove the link files
    fn remove_links(&self, links: &[(Link, PathBuf)]) -> Result<()> {
        for (_link, path) in links {
            // Stage the removal with git rm
            self.git
                .remove(&self.context.working_dir, &path.to_string_lossy())?;
        }
        Ok(())
    }

    /// Commit the removal of a specific link
    fn commit_removal(&self, source: &str, target: &str, count: usize) -> Result<()> {
        let message = format!(
            "unlink(F016): {} -/-> {} ({} link{})",
            source,
            target,
            count,
            if count == 1 { "" } else { "s" }
        );
        self.git_commit(&message)
    }

    /// Commit the removal of all links from a source
    fn commit_removal_all_from(&self, source: &str, count: usize) -> Result<()> {
        let message = format!(
            "unlink(F016): all from {} ({} link{})",
            source,
            count,
            if count == 1 { "" } else { "s" }
        );
        self.git_commit(&message)
    }

    /// Commit the removal of all links to a target
    fn commit_removal_to(&self, target: &str, count: usize) -> Result<()> {
        let message = format!(
            "unlink(F016): all to {} ({} link{})",
            target,
            count,
            if count == 1 { "" } else { "s" }
        );
        self.git_commit(&message)
    }

    /// Create a git commit
    fn git_commit(&self, message: &str) -> Result<()> {
        self.git.commit(&self.context.working_dir, message)
    }
}
