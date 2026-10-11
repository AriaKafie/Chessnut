
#include "bitboard.h"

#include <algorithm>
#ifndef BMI
    #include <string.h>
    #include <random>
#endif
#include <sstream>

Bitboard pdep(Bitboard mask, int pattern)
{
    Bitboard dest = 0;

    for (;pattern; clear_lsb(mask), pattern >>= 1)
        if (pattern & 1) dest |= mask & -mask;

    return dest;
}

Bitboard attacks_bb(PieceType pt, Square sq, Bitboard occupied)
{
    Bitboard  attacks              = 0;
    Direction rook_directions[4]   = { NORTH, EAST, SOUTH, WEST };
    Direction bishop_directions[4] = { NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST };

    for (Direction d : pt == BISHOP ? bishop_directions : rook_directions)
    {
        Square s = sq;

        do
            attacks |= safe_step(s, d);
        while (safe_step(s, d) && !(square_bb(s += d) & occupied));
    }
    
    return attacks;
}

void Bitboards::init()
{
#ifndef BMI
    for (int i = 0; i < 1 << 16; i++)
        for (int j = i; j; j &= j - 1) popcnt16[i]++;
#endif
    for (Square s1 = H1; s1 <= A8; s1++)
    {
        FileBB[s1] = FILE_HBB << s1 % 8;

        for (Square s2 = H1; s2 <= A8; s2++)
            SquareDistance[s1][s2] = std::max(file_distance(s1, s2), rank_distance(s1, s2));
    }
#ifndef BMI
    Bitboard magic, occupied[4096], attacks[4096], *base = pext_table;
#else
    Bitboard *base = pext_table, *xray = xray_table;
#endif
    for (PieceType pt : { BISHOP, ROOK })
        for (Square sq = H1; sq <= A8; sq++)
        {
            Bitboard mask = attacks_bb(pt, sq, 0) &~
                ((FILE_ABB | FILE_HBB) & ~file_bb(sq) | (RANK_1BB | RANK_8BB) & ~rank_bb(sq));

            Magic& m            = Magics[sq][pt - BISHOP];
            int    permutations = 1 << popcount(mask);

            m.ptr  = base;
            m.mask = mask;
#ifdef BMI
            m.xray = xray;

            for (int i = 0; i < permutations; i++)
            {
                Bitboard occupied = pdep(mask, i);

                *base++ = attacks_bb(pt, sq, occupied);
                *xray++ = attacks_bb(pt, sq, occupied ^ attacks_bb(pt, sq, occupied) & occupied);
            }
#else
            bool failed = false;
            m.shift = 64 - popcount(mask);

            for (int p = 0; p < permutations; p++)
            {
                occupied[p] = pdep(mask, p);
                attacks[p]  = attacks_bb(pt, sq, occupied[p]);
            }

            std::mt19937_64 rng(0);

            do
            {
                magic = rng() & rng() & rng();
                memset(base, 0, sizeof(Bitboard) * permutations);

                for (int p = 0, key = 0; p < permutations; key = occupied[++p] * magic >> m.shift)
                {
                    if (failed = base[key] && base[key] != attacks[p])
                        break;

                    base[key] = attacks[p];
                }

            } while (failed);

            m.magic = magic;

            base += permutations;
#endif
        }

    for (Square s1 = H1; s1 <= A8; s1++)
    {
        MainDiag[s1] = bishop_attacks(s1, 0) & (mask(s1, NORTH_WEST) | mask(s1, SOUTH_EAST)) | square_bb(s1);
        AntiDiag[s1] = bishop_attacks(s1, 0) & (mask(s1, NORTH_EAST) | mask(s1, SOUTH_WEST)) | square_bb(s1);

        CenterDistance[s1] = std::min({ rank_distance(s1, E4) + file_distance(s1, E4),
                                        rank_distance(s1, E5) + file_distance(s1, E5),
                                        rank_distance(s1, D4) + file_distance(s1, D4),
                                        rank_distance(s1, D5) + file_distance(s1, D5) });

        for (Square s2 = H1; s2 <= A8; s2++) {
            if (PieceType pt; attacks_bb(pt=BISHOP, s1, 0) & square_bb(s2) || attacks_bb(pt=ROOK, s1, 0) & square_bb(s2)) {
                CheckRay [s1][s2] = attacks_bb(pt, s1, square_bb(s2)) & attacks_bb(pt, s2, square_bb(s1)) | square_bb(s2);
                AlignMask[s1][s2] = attacks_bb(pt, s1, 0)             & attacks_bb(pt, s2, 0)             | square_bb(s1, s2);
            }
        }

        for (Direction d : { NORTH, NORTH_EAST, EAST, SOUTH_EAST, SOUTH, SOUTH_WEST, WEST, NORTH_WEST })
            KingAttacks[s1] |= safe_step(s1, d);

        for (Direction d : { NORTH+NORTH_EAST, NORTH_EAST+EAST, SOUTH_EAST+EAST, SOUTH+SOUTH_EAST,
                             SOUTH+SOUTH_WEST, SOUTH_WEST+WEST, NORTH_WEST+WEST, NORTH+NORTH_WEST })
            KnightAttacks[s1] |= safe_step(s1, d);

        Square sq = make_square(rank_of(s1), FILE_G);

        KMagics[WHITE][s1].mask =
            ((rank_bb(sq + NORTH) | rank_bb(sq + NORTH + NORTH)) & ~(mask(sq + WEST, WEST))) << std::clamp(s1 % 8 - 1, 0, 5) & mask(s1, NORTH);

        KMagics[BLACK][s1].mask =
            ((rank_bb(sq + SOUTH) | rank_bb(sq + SOUTH + SOUTH)) & ~(mask(sq + WEST, WEST))) << std::clamp(s1 % 8 - 1, 0, 5) & mask(s1, SOUTH);

        DoubleCheck[s1] = KingAttacks[s1] | KnightAttacks[s1];

        PawnAttacks[WHITE][s1] = pawn_attacks<WHITE>(square_bb(s1));
        PawnAttacks[BLACK][s1] = pawn_attacks<BLACK>(square_bb(s1));
    }

    for (Color c : { WHITE, BLACK })
    {
        Bitboard relevancy = relative_rank(c, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7);
#ifdef BMI
        Bitboard masks[4] = {
            relevancy & relative_file(c, FILE_G, FILE_H),
            relevancy & relative_file(c, FILE_E, FILE_F),
            relevancy & relative_file(c, FILE_C, FILE_D),
            relevancy & relative_file(c, FILE_A, FILE_B)
        };
#else
        Bitboard masks[4] = {};

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 10; j++) {
                masks[i] |= relevancy & -relevancy;
                clear_lsb(relevancy);
            }
        }
#endif
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 1 << popcount(masks[i]); j++) {

                Bitboard spans = 0;

                for (Bitboard enemy_pawn = pdep(masks[i], j); enemy_pawn; clear_lsb(enemy_pawn)) {
                    Square s = bsf(enemy_pawn);
                    spans |= (file_bb(s) | file_bb(s + EAST) & ~FILE_ABB | file_bb(s + WEST) & ~FILE_HBB) & mask(s, relative_direction(c, SOUTH));
                }

                Passers[c][i][j] = ~spans;
            }
        }
    }

    for (int i = 0; i < 1 << 5; i++)
    {
        const uint8_t clearK = ~8;
        const uint8_t clearQ = ~4;
        const uint8_t cleark = ~2;
        const uint8_t clearq = ~1;

        Bitboard w_occ = pdep(square_bb(A1, E1, H1, A8, H8), i);
        Bitboard b_occ = pdep(square_bb(A8, E8, H8, A1, H1), i);

        uint8_t w_mask = 0xf;
        uint8_t b_mask = 0xf;

        if ((w_occ & square_bb(A1)) == 0) w_mask &= clearQ;
        if ((w_occ & square_bb(E1)) == 0) w_mask &= clearK & clearQ;
        if ((w_occ & square_bb(H1)) == 0) w_mask &= clearK;
        if ((w_occ & square_bb(A8)) != 0) w_mask &= clearq;
        if ((w_occ & square_bb(H8)) != 0) w_mask &= cleark;

        if ((b_occ & square_bb(A8)) == 0) b_mask &= clearq;
        if ((b_occ & square_bb(E8)) == 0) b_mask &= cleark & clearq;
        if ((b_occ & square_bb(H8)) == 0) b_mask &= cleark;
        if ((b_occ & square_bb(A1)) != 0) b_mask &= clearQ;
        if ((b_occ & square_bb(H1)) != 0) b_mask &= clearK;
#ifdef BMI
        CastleMasks[WHITE][i] = w_mask;
        CastleMasks[BLACK][i] = b_mask;
#else
        CastleMasks[WHITE][w_occ * 0x4860104020003061ull >> 59] = w_mask;
        CastleMasks[BLACK][b_occ * 0x1080000400400c21ull >> 59] = b_mask;
#endif
    }

    for (Color c : { WHITE, BLACK })
        for (Square ksq = H1; ksq <= A8; ksq++)
        {
            KMagics[c][ksq].ptr = KingShieldScores[c][ksq];
#ifndef BMI
            Bitboard mask = KMagics[c][ksq].mask, occupied[64], magic;
            bool visited[64] = {}, failed;
            
            int permutations = 1 << popcount(mask);

            for (int p = 0; p < permutations; p++)
                occupied[p] = pdep(mask, p);

            std::mt19937_64 rng(0);

            do
            {
                magic = rng() & rng() & rng();
                
                for (int p = 0, key = 0; p < permutations; key = occupied[++p] * magic >> 64 - popcount(mask))
                {
                    if (failed = visited[key])
                        break;

                    visited[key] = true;
                }

                memset(visited, false, permutations);

            } while (failed);

            KMagics[c][ksq].magic = magic;
#endif
            for (int i = 0; i < 1 << popcount(KMagics[c][ksq].mask); i++)
            {
                Bitboard king_shield = pdep(KMagics[c][ksq].mask, i);
#ifdef BMI
                int& score = KMagics[c][ksq].ptr[i];
#else
                int& score = KMagics[c][ksq].ptr[king_shield * KMagics[c][ksq].magic >> 58];
#endif
                score = 0;
                const int MIN_SCORE = -45, MAX_SCORE = 45;

                if (!(square_bb(ksq) & (c == WHITE ? RANK_1BB : RANK_8BB)) || (popcount(king_shield) < 2))
                {
                    score = MIN_SCORE;
                    continue;
                }

                if (square_bb(ksq) & (FILE_EBB | FILE_DBB))
                {
                    score = 10;
                    continue;
                }

                const int pawn_weights[8][6] =
                { // [H...A][LSB...MSB]
                    { 10, 20, 15, 5, 10, 5 }, { 5,  25, 15, 0, 0,  5 },
                    { 20, 15, 10, 0, 0,  0 }, { 0,  0,  0,  0, 0,  0 },
                    { 0,  0,  0,  0, 0,  0 }, { 10, 15, 20, 0, 0,  0 },
                    { 15, 25, 5,  5, 0,  0 }, { 15, 20, 10, 5, 10, 5 }
                };

                const int file_weights[8][3] =
                { // [H...A][right, middle, left]
                    { 40, 50, 45 }, {40, 50, 45 },
                    { 50, 45, 40 }, {0,  0,  0  },
                    { 0,  0,  0  }, {40, 45, 50 },
                    { 45, 50, 40 }, {45, 50, 40 }
                };

                Bitboard shield_mask  = KMagics[c][ksq].mask;
                Bitboard file_right   = file_bb(bsf(shield_mask));
                Bitboard file_mid     = file_right << 1;
                Bitboard file_left    = file_right << 2;
                int      king_file    = ksq & 7;

                if ((king_shield & file_right) == 0) score -= file_weights[king_file][0];
                if ((king_shield & file_mid)   == 0) score -= file_weights[king_file][1];
                if ((king_shield & file_left)  == 0) score -= file_weights[king_file][2];

                for (int i = 0; shield_mask; clear_lsb(shield_mask), i++)
                {
                    int index = (c == WHITE) ? i : ((i < 3) ? (i + 3) : (i - 3));

                    if (king_shield & square_bb(bsf(shield_mask)))
                        score += pawn_weights[king_file][index];
                }

                score = std::clamp(score, MIN_SCORE, MAX_SCORE);
            }
        }
}

std::string to_string(Bitboard bb)
{
    std::ostringstream os;
    std::string l = "+---+---+---+---+---+---+---+---+";

    for (Rank r = RANK_8; r >= RANK_1; r--)
    {
        os << l << "\n";

        for (File f = FILE_A; f >= FILE_H; f--)
            os << "| " << (bb & square_bb(make_square(r, f)) ? "@" : " ") << " ";

        os << "|\n";
    }

    return os.str() + l;
}
