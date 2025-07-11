#ifndef MINIPROT_H
#define MINIPROT_H

#include <stdint.h>

#define MP_VERSION "0.18-r281"

#define MP_F_NO_SPLICE    0x1
#define MP_F_NO_ALIGN     0x2
#define MP_F_SHOW_UNMAP   0x4
#define MP_F_GFF          0x8
#define MP_F_NO_PAF       0x10
#define MP_F_GTF          0x20
#define MP_F_NO_PRE_CHAIN 0x40
#define MP_F_SHOW_RESIDUE 0x80
#define MP_F_SHOW_TRANS   0x100
#define MP_F_NO_CS        0x200

#define MP_FEAT_CDS       0
#define MP_FEAT_STOP      1

#define MP_BITS_PER_AA    4
#define MP_BLOCK_BONUS    2

#define MP_CODON_STD 1
#define MP_IDX_MAGIC "MPI\3"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { uint64_t x, y; } mp128_t;
typedef struct { int32_t n, m; mp128_t *a; } mp128_v;
typedef struct { int32_t n, m; uint64_t *a; } mp64_v;

typedef struct {
	int32_t bbit; // block bit
	int32_t min_aa_len; // ignore ORFs shorter than this
	int32_t kmer, mod_bit;
	uint32_t trans_code;
} mp_idxopt_t;

typedef struct {
	uint32_t flag;
	int64_t mini_batch_size;
	int32_t max_occ;
	int32_t max_gap; // max gap on the query protein, in aa
	int32_t max_intron;
	int32_t min_max_intron, max_max_intron;
	int32_t bw; // bandwidth, in aa
	int32_t max_ext;
	int32_t max_ava;
	int32_t min_chn_cnt;
	int32_t max_chn_max_skip;
	int32_t max_chn_iter;
	int32_t min_chn_sc;
	float chn_coef_log;
	float mask_level;
	int32_t mask_len;
	float pri_ratio;
	float out_sim, out_cov;
	int32_t best_n, out_n;
	int32_t kmer2;
	int32_t go, ge, io, fs; // gap open, extension, intron open, and frame-shift/stop-codon
	int32_t io_end;
	float ie_coef;
	int32_t sp_model;
	int32_t sp_null_bonus, sp_max_bonus;
	float sp_scale;
	int32_t xdrop;
	int32_t end_bonus;
	int32_t asize; // size of the alphabet; always 22 in current implementation
	int32_t gff_delim;
	int32_t max_intron_flank;
	const char *gff_prefix;
	int8_t mat[484];
} mp_mapopt_t;

typedef struct {
	uint32_t n, m;
	uint64_t *a; // pos<<56 | score<<1 | acceptor
} mp_spsc_t;

typedef struct {
	int64_t off, len;
	char *name;
} mp_ctg_t;

typedef struct {
	int32_t n_ctg, m_ctg;
	int32_t l_name;
	int64_t l_seq, m_seq;
	uint8_t *seq; // TODO: separate this into multiple blocks; low priority
	mp_ctg_t *ctg;
	char *name;
	void *h; // hash table to map sequence name to cid
	mp_spsc_t *spsc; // of size n_ctg*2
} mp_ntdb_t;

typedef struct {
	mp_idxopt_t opt;
	uint32_t n_block;
	mp_ntdb_t *nt;
	int64_t n_kb, *ki;
	uint32_t *bo, *kb;
} mp_idx_t;

typedef struct {
	int32_t dp_score, dp_max, dp_max2;
	int32_t n_cigar, m_cigar;
	int32_t blen; // CDS length in alignment
	int32_t n_fs; // number of frameshift events
	int32_t n_stop; // number of in-frame stop codons
	int32_t dist_stop; // distance in bp to the closest stop codon
	int32_t dist_start; // distance in bp the the closest 'M'
	int32_t n_iden, n_plus;
	uint32_t cigar[];
} mp_extra_t;

typedef struct {
	int64_t vs, ve;
	int32_t qs, qe;
	int16_t type, phase;
	int32_t n_fs, n_stop;
	int32_t score, n_iden, blen;
	char donor[2], acceptor[2];
} mp_feat_t;

typedef struct {
	int32_t off, cnt;
	int32_t id, parent;
	int32_t n_sub, subsc;
	int32_t n_feat, m_feat, n_exon;
	int32_t chn_sc;
	int32_t chn_sc_ungap;
	uint32_t hash;
	uint32_t vid; // cid<<1 | rev
	int32_t qs, qe;
	int64_t vs, ve;
	uint64_t *a;
	mp_feat_t *feat;
	mp_extra_t *p;
} mp_reg1_t;

struct mp_tbuf_s;
typedef struct mp_tbuf_s mp_tbuf_t;

extern int32_t mp_verbose, mp_dbg_flag;
extern char *ns_tab_nt_i2c, *ns_tab_aa_i2c;
extern uint8_t ns_tab_a2r[22], ns_tab_nt4[256], ns_tab_aa20[256], ns_tab_aa13[256];
extern uint8_t ns_tab_codon[64], ns_tab_codon13[64];

/**
 * Set the translation table and builtin timer
 *
 * Run this before all other miniprot routines.
 */
void mp_start(void);

/**
 * Initialize the default parameters for indexing
 *
 * @param io       struct to initialize (out)
 */
void mp_idxopt_init(mp_idxopt_t *io);

/**
 * Initialize the default parameters fr mapping
 *
 * @param mo       struct to initialize (out)
 */
void mp_mapopt_init(mp_mapopt_t *mo);

/**
 * Set frameshift and stop codon penalty
 *
 * This function changes the mp_mapopt_t::fs field and mp_mapopt_t::mat[]
 *
 * @param mo       mapping options (in/out)
 * @param fs       frameshift and stop codon penalty (positive)
 */
void mp_mapopt_set_fs(mp_mapopt_t *mo, int32_t fs);

/**
 * Set max intron length based on the genome size
 *
 * @param mo       mapping options (in/out)
 * @param gsize    genome size
 */
void mp_mapopt_set_max_intron(mp_mapopt_t *mo, int64_t gsize);

/**
 * Check the integrity of mapping parameters (not complete)
 *
 * @param mo       mapping options
 *
 * @return 0 on success, or negative on failure
 */
int32_t mp_mapopt_check(const mp_mapopt_t *mo);

/**
 * Load or build an index
 *
 * If _fn_ corresponds to a FASTA file, build the index using _io_ and
 * _n_threads_. If _fn_ corresponds to a prebuilt index file, load the index
 * and ignore _io_ and _n_threads_.
 *
 * @param fn         FASTA or index file name
 * @param io         indexing options
 * @param n_threads  number of threads
 *
 * @return the index
 */
mp_idx_t *mp_idx_load(const char *fn, const mp_idxopt_t *io, int32_t n_threads);

/**
 * Deallocate the index
 *
 * @param mi         the index
 */
void mp_idx_destroy(mp_idx_t *mi);

/**
 * Write an index to disk
 *
 * @param fn         index file name
 * @param mi         the index
 *
 * @return 0 on success or negative on failure
 */
int mp_idx_dump(const char *fn, const mp_idx_t *mi);

/**
 * Read an index file
 *
 * @param fn         index file name
 *
 * @return the index, or NULL on failure
 */
mp_idx_t *mp_idx_restore(const char *fn);

/**
 * Load a splice score file
 *
 * @param nt        sequences in mp_idx_t::nt (in/out)
 * @param fn        splice score file name; can be optionally gzip'd
 * @param max_sc    cap splice scores at this parameter
 *
 * @return 0 on success, or negative on failure
 */
int32_t mp_ntseq_read_spsc(mp_ntdb_t *nt, const char *fn, int32_t max_sc);

void mp_set_spsc(const char *fn, mp_idx_t *mi, mp_mapopt_t *mo, int32_t keep_io);

/**
 * Align a protein sequence against the index
 *
 * @param mi        the index
 * @param qlen      protein length
 * @param seq       protein sequence
 * @param n_reg     number of alignments (out)
 * @param b         thread-local buffer; threads shouldn't share buffers (can be NULL)
 * @param opt       mapping parameters
 * @param qname     query name (used for breaking ties)
 *
 * @return list of alignments
 */
mp_reg1_t *mp_map(const mp_idx_t *mi, int qlen, const char *seq, int *n_reg, mp_tbuf_t *b, const mp_mapopt_t *opt, const char *qname);

/**
 * Initialize a thread-local buffer
 *
 * @return the buffer
 */
mp_tbuf_t *mp_tbuf_init(void);

/**
 * Deallocate a thread-local buffer
 *
 * @param b         the buffer
 */
void mp_tbuf_destroy(mp_tbuf_t *b);

// ignore the following for now
void mp_idx_print_stat(const mp_idx_t *mi, int32_t max_occ);
int32_t mp_map_file(const mp_idx_t *idx, const char *fn, const mp_mapopt_t *opt, int n_threads);

#ifdef __cplusplus
}
#endif

#endif
