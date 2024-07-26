#include "IStreamQuadIterator.hpp"
#include <rdf4cpp/parser/IStreamQuadIteratorSerdImpl.hpp>

#include <cstdio>

#if __has_include(<fcntl.h>)
#include <fcntl.h>
#endif //__has_include

namespace rdf4cpp::parser {

/**
 * Adaptor function so that serd can read from std::istreams.
 * Matches the interface of SerdSource/fread
 *
 * @param buf pointer to buffer to be written to
 * @param elem_size sizeof each element being written, guaranteed to always be 1
 * @param count number of elements to write
 * @param voided_self pointer to std::istream cast to void *
 */
static size_t istream_read(void *buf, [[maybe_unused]] size_t elem_size, size_t count, void *voided_self) noexcept {
    assert(elem_size == 1);

    auto *self = static_cast<std::istream *>(voided_self);
    self->read(static_cast<char *>(buf), static_cast<std::streamsize>(count));
    return self->gcount();
}

/**
 * Adaptor function for serd to check if an std::istream is ok
 * Matches the interface of SerdStreamErrorFunc
 *
 * @param voided_self pointer to std::istream cast to void *
 * @return whether the given istream encountered an error (cast to int)
 */
static int istream_error(void *voided_self) noexcept {
    auto *self = static_cast<std::istream *>(voided_self);
    return static_cast<int>(self->fail() && !self->eof());
}

IStreamQuadIterator::IStreamQuadIterator(void *stream,
                                         ReadFunc read,
                                         ErrorFunc error,
                                         flags_type flags,
                                         state_type *state)
    : impl{std::make_unique<Impl>(stream, read, error, flags, state)},
      cur{impl->next()} {
}

IStreamQuadIterator::IStreamQuadIterator(std::istream &istream,
                                         flags_type flags,
                                         state_type *state) noexcept
    : IStreamQuadIterator{&istream, &istream_read, &istream_error, flags, state} {
}

IStreamQuadIterator::IStreamQuadIterator(IStreamQuadIterator &&other) noexcept = default;
IStreamQuadIterator &IStreamQuadIterator::operator=(IStreamQuadIterator &&) noexcept = default;

IStreamQuadIterator::~IStreamQuadIterator() noexcept = default;

typename IStreamQuadIterator::reference IStreamQuadIterator::operator*() const noexcept {
    return *cur;
}

typename IStreamQuadIterator::pointer IStreamQuadIterator::operator->() const noexcept {
    return &*cur;
}

IStreamQuadIterator &IStreamQuadIterator::operator++() {
    cur = impl->next();
    return *this;
}

uint64_t IStreamQuadIterator::current_line() const noexcept {
    return impl->current_line();
}

uint64_t IStreamQuadIterator::current_column() const noexcept {
    return impl->current_column();
}

bool IStreamQuadIterator::operator==(std::default_sentinel_t) const noexcept {
    return !cur.has_value();
}

bool IStreamQuadIterator::operator!=(std::default_sentinel_t) const noexcept {
    return cur.has_value();
}

FILE *fopen_fastseq(char const *path, char const *mode) noexcept {
    // inspired by <serd/system.c> (serd_fopen)

    FILE *fd = fopen(path, mode);
    if (fd == nullptr) {
        return fd;
    }

#if __has_include(<fcntl.h>) && _POSIX_C_SOURCE >= 200112L
    (void) posix_fadvise(fileno(fd), 0, 0, POSIX_FADV_SEQUENTIAL | POSIX_FADV_NOREUSE | POSIX_FADV_WILLNEED);
#endif

    return fd;
}

}  // namespace rdf4cpp::parser
