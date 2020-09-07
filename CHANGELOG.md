# Changelog

This project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

The structure and content of this file follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [4.0.0] - 2020-09-07
### Changed
- Almost everything has changed. Same approach but different
  implemenation. A `oj_` prefix is used instead of `ojc_` to make the
  change over easier.

## [3.1.1] - 2017-11-19
### Changed
- Reverted len arg to int from size_t for create string.

## [3.1.0] - 2017-11-11
### Added
- Added functions to support the external wire format.
### Removed
- Removed wire format which is now in the `ohler55/opo-c` github repository.

## [3.0.0] - 2017-11-8
### Changed
- Reorganized files and directory structure.
- Renamed parse functions to match argument better.
### Added
- Added wire format for faster transmission of and stream handling.

## [2.1.0] - 2017-08-14
### Fixed
- Updated buffer (buf.h) support.
- Added check for double free.

## [2.0.1] - 2017-04-26
### Fixed
- Multiple fixes and minor changes to the API.

## [1.13.0] - 2015-09-21
### Added
- Added functions to remove elements by path.

## [1.12.0] - 2015-09-3
### Added
- Added support for opaque elements that can be set but do not get
  written. They do not show up in string output unless the new
  ojc_write_opaque is set to true.

## [1.11.0] - 2015-08-15
### Fixed
- Fixed bug in ocj_get() that missed keys of a medium size range.
### Added
- Added an option for case insensitive searches. By setting the
  ojc_case_insensitive flag to true search and replace functions
  become case insensitive.

## [1.10.0] - 2015-05-17
### Added
- Added support for arrays in ojc_replace().
- Added ojc_array_remove(), ojc_array_replace(), ojc_array_insert().
- Aliased ojc_set() to ojc_replace() and ojc_aset() to ojc_areplace().
### Changed
- Changed ojc_object_remove_by_pos() to ojc_remove_by_pos() which now
  supports arrays.

## [1.9.0] - 2015-04-14
### Changed
- Changed ojc_object_replace to return a boolean.
### Added
- Added ojc_append() and ojc_aappend() for appending to a tree with a path.
- Added ojc_replace() and ojc_areplace() for replacing or append to a tree with a path.

## [1.8.0] - 2015-04-7
### Added
- Added ojc_cmp() function that compares two ojcVal values.
### Fixed
- Fixed follow parser to remain open as expected.

## [1.7.0] - 2015-January-1
### Added
- Added ojc_duplicate() function that make a deep copy of an element (ojcVal).

## [1.6.0] - 2014-12-12
### Added
- Added option to parse decimal numbers as strings instead of doubles.
- Added ojc_number() function.

## [1.5.0] - 2014-10-23
### Added
- Added parse function where the caller provides a read
  function. Planned for use with external libraries such as zlib.

## [1.4.6] - 2014-10-7
### Fixed
- Cleaned up ubuntu/g++ errors.

## [1.4.5] - 2014-09-26
### Fixed
- Fixed memory leak with number strings.

## [1.4.4] - 2014-09-22
### Added
- Added an unbuffered parse function to work with tail -f.

## [1.4.3] - 2014-08-29
### Fixed
- Attempting to extract from a NULL val no longer crashes.

## [1.4.2] - 2014-08-28
### Fixed
- Fixed bug in parse destroy during callback parsing.

## [1.4.1] - 2014-08-21
### Added
- Added cleanup function to free memory used in re-use pools.

## [1.4.0] - 2014-08-15
### Added
- Added functions to get members of an object and an array.

## [1.3.0] -2014-07-31
### Added
- Added support for non quoted words as values if a flag is set to
  allow the non-standard feature. Functions create and get values from
  word values also added.

## [1.2.5] -2014-07-21
### Added
- Added object functions for changing the members of the object.

## [1.2.4] -2014-07-6
### Added
- Added function to set the key of a value.

## [1.2.3] -2014-06-26
### Fixed
- Fixed buffer overflow by 1 error.

## [1.2.2] -2014-06-25
### Added
- Allow raw \n in output when ojc newline ok is true.

## [1.2.1] -2014-06-20
### Added
- Allow raw \n in output.

## [1.2.0] -2014-06-10
### Added
- Added push and pop to array values.

## [1.1.0] - 2014-05-28
### Fixed
- Fixed ubuntu compilation.
### Changed
- Changed callback for parsing to allow the parsing to be aborted by the callback.

## [1.0.2] - 2014-05-28
### Added
- Added static initializer for ojcErr.
- Added ojc_str().

## [1.0.1] - 2014-05-27
### Fixed
- Corrected compile errors with clang and made ojc.h C++ compatible.

## [1.0.0] - 2014-05-26
### Added
- Initial release.
