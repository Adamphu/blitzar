#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SXT_CPU_BACKEND 1
#define SXT_GPU_BACKEND 2

/** config struct to hold the chosen backend **/
struct sxt_config {
  int backend;
  uint64_t num_precomputed_generators;
};

struct sxt_compressed_ristretto {
  // encodes an element of the ristretto255 group
  uint8_t ristretto_bytes[32];
};

struct sxt_scalar {
  // encodes an element of the finite field modulo (2^252 + 27742317777372353535851937790883648493)
  uint8_t bytes[32];
};

struct sxt_transcript {
  // encodes a strobe-based transcript
  uint8_t bytes[203];
};

struct sxt_ristretto {
  // encodes an element of the curve255 group
  uint64_t X[5];
  uint64_t Y[5];
  uint64_t Z[5];
  uint64_t T[5];
};

/** describes a sequence of values **/
struct sxt_sequence_descriptor {
  // the number of bytes used to represent an element in the sequence
  // element_nbytes must be a power of 2 and must satisfy 1 <= element_nbytes <= 32
  uint8_t element_nbytes;

  // the number of elements in the sequence
  uint64_t n;

  // pointer to the data for the sequence of elements where there are n elements
  // in the sequence and each element encodes a number of element_nbytes bytes
  // represented in the little endian format
  const uint8_t* data;

  // whether the elements are signed
  // Note: if signed, then element_nbytes must be <= 16
  int is_signed;
};

/**
 * Initialize the library. This should only be called once.
 *
 * Arguments:
 *
 * - config (in): specifies which backend should be used in the computations. Those
 *   available are: SXT_GPU_BACKEND, and SXT_CPU_BACKEND
 *
 * # Return:
 *
 * - 0 on success; otherwise a nonzero error code
 */
int sxt_init(const struct sxt_config* config);

/**
 * Compute the pedersen commitments for sequences of values
 *
 * Denote an element of a sequence by a_ij where i represents the sequence index
 * and j represents the element index. Let * represent the operator for the
 * ristretto255 group. Then res\[i] encodes the ristretto255 group value
 *
 * ```text
 *     Prod_{j=1 to n_i} g_{offset_generators + j} ^ a_ij
 * ```
 *
 * where n_i represents the number of elements in sequence i and g_{offset_generators + j}
 * is a group element determined by a prespecified function
 *
 * ```text
 *     g: uint64_t -> ristretto255
 * ```
 *
 * # Arguments:
 *
 * - commitments   (out): an array of length num_sequences where the computed commitments
 *                     of each sequence must be written into
 *
 * - num_sequences (in): specifies the number of sequences
 * - descriptors   (in): an array of length num_sequences that specifies each sequence
 * - offset_generators (in): specifies the offset used to fetch the generators
 *
 * # Abnormal program termination in case of:
 *
 * - backend not initialized or incorrectly initialized
 * - descriptors == nullptr
 * - commitments == nullptr
 * - descriptor\[i].element_nbytes == 0
 * - descriptor\[i].element_nbytes > 32
 * - descriptor\[i].n > 0 && descriptor\[i].data == nullptr
 *
 * # Considerations:
 *
 * - num_sequences equal to 0 will skip the computation
 */
void sxt_compute_pedersen_commitments(struct sxt_compressed_ristretto* commitments,
                                      uint32_t num_sequences,
                                      const struct sxt_sequence_descriptor* descriptors,
                                      uint64_t offset_generators);

/**
 * Compute the pedersen commitments for sequences of values
 *
 * Denote an element of a sequence by a_ij where i represents the sequence index
 * and j represents the element index. Let * represent the operator for the
 * ristretto255 group. Then res\[i] encodes the ristretto255 group value
 *
 * ```text
 *     Prod_{j=1 to n_i} g_j ^ a_ij
 * ```
 *
 * where n_i represents the number of elements in sequence i and g_j is a group
 * element determined by the `generators\[j]` user value given as input
 *
 * # Arguments:
 *
 * - commitments   (out): an array of length num_sequences where the computed commitments
 *                     of each sequence must be written into
 *
 * - num_sequences (in): specifies the number of sequences
 * - descriptors   (in): an array of length num_sequences that specifies each sequence
 * - generators    (in): an array of length `max_num_rows` = `the maximum between all n_i`
 *
 * # Abnormal program termination in case of:
 *
 * - backend not initialized or incorrectly initialized
 * - descriptors == nullptr
 * - commitments == nullptr
 * - descriptor\[i].element_nbytes == 0
 * - descriptor\[i].element_nbytes > 32
 * - descriptor\[i].n > 0 && descriptor\[i].data == nullptr
 *
 * # Considerations:
 *
 * - num_sequences equal to 0 will skip the computation
 */
void sxt_compute_pedersen_commitments_with_generators(
    struct sxt_compressed_ristretto* commitments, uint32_t num_sequences,
    const struct sxt_sequence_descriptor* descriptors, const struct sxt_ristretto* generators);

/**
 * Gets the pre-specified random generated elements used for the Pedersen commitments in the
 * `sxt_compute_pedersen_commitments` function
 *
 * sxt_get_generators(generators, num_generators, offset_generators) →
 *     generators\[0] = generate_random_ristretto(0 + offset_generators)
 *     generators\[1] = generate_random_ristretto(1 + offset_generators)
 *     generators\[2] = generate_random_ristretto(2 + offset_generators)
 *       .
 *       .
 *       .
 *     generators\[num_generators - 1] = generate_random_ristretto(num_generators - 1 +
 * offset_generators)
 *
 * # Arguments:
 *
 * - generators         (out): sxt_element_p3 pointer where the results must be written into
 *
 * - num_generators     (in): the total number of random generated elements to be computed
 * - offset_generators  (in): the offset that shifts the first element computed from `0` to
 * `offset_generators`
 *
 * # Return:
 *
 * - 0 on success; otherwise a nonzero error code
 *
 * # Invalid input parameters, which generate error code:
 *
 * - num_generators > 0 && generators == nullptr
 *
 * # Considerations:
 *
 * - num_generators equal to 0 will skip the computation
 */
int sxt_get_generators(struct sxt_ristretto* generators, uint64_t offset_generators,
                       uint64_t num_generators);

/**
 * Gets the n-th ristretto point defined as:
 *
 * If n == 0:
 *    one_commit[0] = ristretto_identity;
 *
 * Else:
 *    one_commit[0] = g[0] + g[1] + ... + g[n - 1];
 *
 * where
 *
 * struct sxt_ristretto ristretto_identity = {
 *    {0, 0, 0, 0, 0},
 *    {1, 0, 0, 0, 0},
 *    {1, 0, 0, 0, 0},
 *    {0, 0, 0, 0, 0},
 * };
 *
 * and `g[i]` is the i-th generator provided by `sxt_get_generators` function at offset 0.
 *
 * # Return:
 *
 * - 0 on success; otherwise a nonzero error code
 *
 * # Invalid input parameters, which generate error code:
 *
 * - one_commit == nullptr
 */
int sxt_get_one_commit(struct sxt_ristretto* one_commit, uint64_t n);

/**
 * Creates an inner product proof
 *
 * The proof is created with respect to the base G, provided by
 * `sxt_get_generators(G, generators_offset, 1ull << ceil(log2(n)))`.
 *
 * The `verifier` transcript is passed in as a parameter so that the
 * challenges depend on the *entire* transcript (including parent
 * protocols).
 *
 * Note that we don't have any restriction to the `n` value, other than
 * it has to be non-zero.
 *
 * # Algorithm description
 *
 * Initially, we compute G and Q = G[np], where np = 1ull << ceil(log2(n))
 * and G is zero-indexed.
 *
 * The protocol consists of k = ceil(lg_2(n)) rounds, indexed by j = k - 1 , ... , 0.
 *
 * In the j-th round, the prover computes:
 *
 * ```text
 * a_lo = {a[0], a[1], ..., a[n / 2 - 1]}
 * a_hi = {a[n/2], a[n/2 + 1], ..., a[n - 1]}
 * b_lo = {b[0], b[1], ..., b[n / 2 - 1]}
 * b_hi = {b[n/2], b[n/2 + 1], ..., b[n - 1]}
 * G_lo = {G[0], G[1], ..., G[n / 2 - 1]}
 * G_hi = {G[n/2], G[n/2 + 1], ..., G[n-1]}
 *
 * l_vector[j] = <a_lo, G_hi> + <a_lo, b_hi> * Q
 * r_vector[j] = <a_hi, G_lo> + <a_hi, b_lo> * Q
 * ```
 *
 * Note that if the `a` or `b` length is not a power of 2,
 * then `a` or `b` is padded with zeros until it has a power of 2.
 * G always has a power of 2 given how it is constructed.
 *
 * Then the prover sends l_vector[j] and r_vector[j] to the verifier,
 * and the verifier responds with a
 * challenge value u[j] <- Z_p (finite field of order p),
 * which is non-interactively simulated by
 * the input strobe-based transcript:
 *
 * ```text
 * transcript.append("L", l_vector[j]);
 * transcript.append("R", r_vector[j]);
 *
 * u[j] = transcript.challenge_value("x");
 * ```
 *
 * Then the prover uses u[j] to compute
 *
 * ```text
 * a = a_lo * u[j] + (u[j]^-1) * a_hi;
 * b = b_lo * (u[j]^-1) + u[j] * b_hi;
 * ```
 *
 * Then, the prover and verifier both compute
 *
 * ```text
 * G = G_lo * (u[j]^-1) + u[j] * G_hi
 *
 * n = n / 2;
 * ```
 *
 * and use these vectors (all of length 2^j) for the next round.
 *
 * After the last (j = 0) round, the prover sends ap_value = a[0] to the verifier.
 *
 * # Arguments:
 *
 * - l_vector (out): transcript point array with length `ceil(log2(n))`
 * - r_vector (out): transcript point array with length `ceil(log2(n))`
 * - ap_value (out): a single scalar
 * - transcript (in/out): a single strobe-based transcript
 * - n (in): non-zero length for the input arrays
 * - generators_offset (in): offset used to fetch the bases
 * - a_vector (in): array with length n
 * - b_vector (in): array with length n
 *
 * # Abnormal program termination in case of:
 *
 * - transcript, ap_value, b_vector, or a_vector is nullptr
 * - n is zero
 * - n is non-zero, but l_vector or r_vector is nullptr
 */
void sxt_prove_inner_product(struct sxt_compressed_ristretto* l_vector,
                             struct sxt_compressed_ristretto* r_vector, struct sxt_scalar* ap_value,
                             struct sxt_transcript* transcript, uint64_t n,
                             uint64_t generators_offset, const struct sxt_scalar* a_vector,
                             const struct sxt_scalar* b_vector);

/**
 * Verifies an inner product proof
 *
 * The proof is verified with respect to the base G, provided by
 * `sxt_get_generators(G, generators_offset, 1ull << ceil(log2(n)))`.
 *
 * Note that we don't have any restriction to the `n` value, other than
 * it has to be non-zero.
 *
 * # Arguments:
 *
 * - transcript (in/out): a single strobe-based transcript
 * - n (in): non-zero length for the input arrays
 * - generators_offset (in): offset used to fetch the bases
 * - b_vector (in): array with length n, the same one used by `sxt_prove_inner_product`
 * - product (in): a single scalar, represented by <a, b>,
 *                 the inner product of the two vectors `a` and `b` used by
 * `sxt_prove_inner_product`
 * - a_commit (in): a single ristretto point, represented by <a, G> (the inner product of the two
 * vectors)
 * - l_vector (in): transcript point array with length `ceil(log2(n))`, generated by
 * `sxt_prove_inner_product`
 * - r_vector (in): transcript point array with length `ceil(log2(n))`, generated by
 * `sxt_prove_inner_product`
 * - ap_value (in): a single scalar, generated by `sxt_prove_inner_product`
 *
 * # Return:
 *
 * - 1 in case the proof can be verified; otherwise, return 0
 *
 * # Abnormal program termination in case of:
 *
 * - transcript, ap_value, product, a_commit, or b_vector is nullptr
 * - n is zero
 * - n is non-zero, but l_vector or r_vector is nullptr
 */
int sxt_verify_inner_product(struct sxt_transcript* transcript, uint64_t n,
                             uint64_t generators_offset, const struct sxt_scalar* b_vector,
                             const struct sxt_scalar* product, const struct sxt_ristretto* a_commit,
                             const struct sxt_compressed_ristretto* l_vector,
                             const struct sxt_compressed_ristretto* r_vector,
                             const struct sxt_scalar* ap_value);

#ifdef __cplusplus
} // extern "C"
#endif
