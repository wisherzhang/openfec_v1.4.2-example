/* $Id: of_galois_field_code.c 185 2014-07-15 09:57:16Z roca $ */
/*
 * OpenFEC.org AL-FEC Library.
 * (c) Copyright 2009 - 2012 INRIA - All rights reserved
 * Contact: vincent.roca@inria.fr
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

#include "../of_reed-solomon_gf_2_m_includes.h"


#ifdef OF_USE_REED_SOLOMON_2_M_CODEC


gf of_modnn(of_galois_field_code_cb_t* ofcb,INT32 x)
{
	UINT16 field_size = ofcb->field_size;
	while (x >= field_size)
	{
		x -= field_size;
		x = (x >> ofcb->m) + (x & field_size);
	}
	return x;
}

void of_rs_2m_display_gf(of_galois_field_code_cb_t* ofcb)
{
	OF_ENTER_FUNCTION
	int i;
	for (i = 0; i <= ofcb->field_size; i++) {
		printf("i=%i,log(i)=%i,exp(i)=%i,exp(log(i))=%i\n",i,ofcb->of_rs_gf_log[i],ofcb->of_rs_gf_exp[i],ofcb->of_rs_gf_exp[ofcb->of_rs_gf_log[i]]);
	}
	OF_EXIT_FUNCTION
}


/*
 * shuffle move src packets in their position
 */
static int
of_rs_2m_shuffle (gf *pkt[], gf *pkthead[],int index[], int k)
{
 	OF_ENTER_FUNCTION
 	int i;

 	for (i = 0 ; i < k ;)
 	{
 		if (index[i] >= k || index[i] == i)
 			i++ ;
 		else
 		{
 			/*
 			 * put pkt in the right position (first check for conflicts).
 			 */
 			int c = index[i] ;

 			if (index[c] == c)
 			{
 				OF_EXIT_FUNCTION
 				return 1 ;
 			}
 			SWAP (index[i], index[c], int) ;
 			SWAP (pkt[i], pkt[c], gf *) ;
			SWAP (pkthead[i], pkthead[c], gf *) ;
 		}
 	}
 	OF_EXIT_FUNCTION
 	return 0 ;
}


void		of_rs_2m_release(of_galois_field_code_cb_t* ofcb)
{
	 OF_ENTER_FUNCTION
	 if (ofcb->enc_matrix != NULL)
	 {
		 of_free(ofcb->enc_matrix);
		 ofcb->enc_matrix=NULL;
	 }
	 if (ofcb->dec_matrix != NULL)
	 {
		 of_free(ofcb->dec_matrix);
		 ofcb->dec_matrix=NULL;
	 }
	 OF_EXIT_FUNCTION
}


of_status_t of_rs_2m_build_encoding_matrix(of_galois_field_code_cb_t* ofcb)
{
	OF_ENTER_FUNCTION
	gf *tmp_m, *p;
	UINT32 k,r,col,row;
	k=ofcb->nb_source_symbols;
	r = ofcb->nb_repair_symbols;
	if ((ofcb->enc_matrix = of_malloc((k+r)*(k))) == NULL)
	{
		goto no_mem;
	}
	/* cast the pointer (a 64 bit integer on LP64 systems) to uintptr_t first, and then
	 * truncate it to keep only the lowest 32 bits. Works the same on both 32 bit and 64
	 * bit systems */
	ofcb->magic = ( (FEC_MAGIC ^ k) ^ (k+r)) ^ ((uintptr_t)(ofcb->enc_matrix) & 0xFFFFFFFF);

	if ((tmp_m = of_malloc((k+r)*(k))) == NULL)
	{
		goto no_mem;
	}
	/*
	 * fill the matrix with powers of field elements, starting from 0.
	 * The first row is special, cannot be computed with exp. table.
	 */
	tmp_m[0] = 1 ;
	for (col = 1; col < k ; col++)
		tmp_m[col] = 0 ;
	for (p = tmp_m + k, row = 0; row < k+r - 1 ; row++, p += k)
	{
		for (col = 0 ; col < k ; col ++)
			switch(ofcb->m) {
				case 4:
					p[col] = of_gf_2_4_exp[of_modnn (ofcb,row*col) ];
					break;
				case 8:
					p[col] = of_gf_2_8_exp[of_modnn (ofcb,row*col) ];
					break;					
			}
	}

	/*
	 * quick code to build systematic matrix: invert the top
	 * k*k vandermonde matrix, multiply right the bottom n-k rows
	 * by the inverse, and construct the identity matrix at the top.
	 */
	switch(ofcb->m) {
		case 4:
			of_galois_field_2_4_invert_vdm(ofcb, tmp_m, k);
			of_galois_field_2_4_matmul(tmp_m + k*k, tmp_m, ofcb->enc_matrix + k*k, r, k, k);
			break;
		case 8:
			of_galois_field_2_8_invert_vdm(ofcb, tmp_m, k);
			of_galois_field_2_8_matmul(tmp_m + k*k, tmp_m, ofcb->enc_matrix + k*k, r, k, k);
			break;			
	}
	/*
	 * the upper matrix is I so do not bother with a slow multiply
	 */
	bzero (ofcb->enc_matrix, k*k*sizeof (gf));
	for (p = ofcb->enc_matrix, col = 0 ; col < k ; col++, p += k + 1)
		*p = 1 ;

	of_free (tmp_m);

	OF_EXIT_FUNCTION
	return OF_STATUS_OK;
	no_mem:
		OF_PRINT_ERROR(("out of memory\n"));
	error:
		OF_EXIT_FUNCTION
		return OF_STATUS_FATAL_ERROR;
}


#ifdef OF_USE_DECODER
of_status_t of_rs_2m_build_decoding_matrix(of_galois_field_code_cb_t* ofcb, int *index)
{
	OF_ENTER_FUNCTION
	UINT32 k,r,i;
	gf *p;
	k = ofcb->nb_source_symbols;
	r = ofcb->nb_repair_symbols;
	if ((ofcb->dec_matrix = of_malloc((k)*(k))) == NULL)
	{
		goto no_mem;
	}
	for (i = 0, p = ofcb->dec_matrix ; i < k ; i++, p += k)
	{
#if 1 /* this is simply an optimization, not very useful indeed */
		if (index[i] < k)
		{
			bzero (p, k*sizeof (gf));
			p[i] = 1 ;
		}
		else
#endif
			if (index[i] < (k+r))
				bcopy (& (ofcb->enc_matrix[index[i]*k]), p, k*sizeof (gf));
			else
			{
				OF_PRINT_ERROR ( ("decode: invalid index %d (max %d)\n",
					     index[i], k+r - 1))
				of_free (ofcb->dec_matrix);
				OF_EXIT_FUNCTION
				return OF_STATUS_FATAL_ERROR ;
			}
	}
	int result;
	switch (ofcb->m)
	{
	case 4:
		result = of_galois_field_2_4_invert_mat(ofcb, ofcb->dec_matrix, k);
		break;
	case 8:
		result = of_galois_field_2_8_invert_mat(ofcb, ofcb->dec_matrix, k);
		break;			
	}
	if (result)
	{
		of_free (ofcb->dec_matrix);
		ofcb->dec_matrix = NULL ;
	}
	OF_EXIT_FUNCTION
	return OF_STATUS_OK;

no_mem:
	OF_PRINT_ERROR(("out of memory\n"));
error:
	OF_EXIT_FUNCTION
	return OF_STATUS_FATAL_ERROR;
}


of_status_t of_rs_2m_decode (of_galois_field_code_cb_t* ofcb, gf *_pkt[], gf *_pktead[], int index[], int sz, int headsz)
{
	OF_ENTER_FUNCTION
	gf **pkt = (gf**) _pkt;				/* VR */
	gf **pkthead = (gf**) _pktead;				/* VR */
	//gf **new_pkt ;
	void	**ass_buf;
	void	**ass_headbuf;
	int row, col, k = ofcb->nb_source_symbols ;
	int size     = sz;
	int headsize = headsz;

	if (ofcb->m > 8){
		size /= 2 ;
		headsize /= 2;
	}

	if (of_rs_2m_shuffle (pkt, pkthead, index, k))
	{
		/* error if true */
		OF_EXIT_FUNCTION
		return OF_STATUS_ERROR ;
	}
	if (of_rs_2m_build_decoding_matrix (ofcb, index) != OF_STATUS_OK)
	{
		OF_PRINT_ERROR(("of_rs_2m_decode : cannot build decoding matrix."))
		goto error;
	}

	/*
	 * do the actual decoding
	 */
	//new_pkt = (gf **) of_malloc (k * sizeof (gf *));
	for (row = 0 ; row < k ; row++)
	{
		if (index[row] >= k)
		{
			ass_buf = &(ofcb->available_symbols_tab[row]);
			ass_headbuf = &(ofcb->available_symbolheads_tab[row]);
			OF_PRINT_LVL(2,("decode row %d index %d ass_buf %p\n",
			    row, index[row],ass_buf));
			OF_PRINT_LVL(2,("decode row %d index %d ass_buf %p ass_headbuf%p\n",
			    row, index[row],*ass_buf, *ass_headbuf));
			if(NULL == *ass_buf){
				if (ofcb->decoded_source_symbol_callback != NULL)
				{
					*ass_buf = ofcb->decoded_source_symbol_callback (ofcb->context_4_callback,
						sz, row);
					if(headsz != 0){
						*ass_headbuf = ofcb->decoded_source_symbol_callback (ofcb->context_4_callback,
						headsz, row);
					}
				}
                else if (ofcb->decoded_source_symbol_callback_new != NULL) {
                    ofcb->decoded_source_symbol_callback_new(ofcb->context_4_callback, 
                            ass_buf, ass_headbuf, sz, headsz, row);
                }
				else
				{
					*ass_buf = (void *) of_malloc (sz);
					bzero (*ass_buf, sz * sizeof (gf)) ;
					if(headsz != 0){
						*ass_headbuf = (void *) of_malloc(headsz);
						bzero (*ass_headbuf, headsz * sizeof (gf)) ;
					}
				}
				if (*ass_buf == NULL)
				{
				     OF_PRINT_ERROR(("decode row %d index %d ass_buf %p\n",
			         row, index[row],*ass_buf));
					goto error;
				}
				if(headsz != 0 && *ass_headbuf == NULL){
				     OF_PRINT_ERROR(("decode row %d index %d ass_headbuf %p\n",
			         row, index[row],*ass_headbuf));
					goto error;
				}
			}
			//new_pkt[row] = (gf *) of_calloc (sz, sizeof (gf));
			for (col = 0 ; col < k ; col++)
			{
				if (ofcb->dec_matrix[row*k + col] != 0)
				{
					switch (ofcb->m)
					{
					case 4:
						of_galois_field_2_4_addmul1_compact((gf *)*ass_buf, pkt[col], ofcb->dec_matrix[row*k + col], size);
						if(headsz > 0){
							of_galois_field_2_4_addmul1_compact((gf *)*ass_headbuf, pkthead[col], ofcb->dec_matrix[row*k + col], headsize);
							}
						break;
					case 8:
						// no addmul1 compact form for GF(2^8)
						of_galois_field_2_8_addmul1((gf *)*ass_buf, pkt[col], ofcb->dec_matrix[row*k + col], size);
						if(headsz > 0){
							of_galois_field_2_8_addmul1((gf *)*ass_headbuf, pkthead[col], ofcb->dec_matrix[row*k + col], headsize);
							}
						break;							
					}
				}
			}
			Of_SaveFile("/mnt/orig_dec0", (gf **)ass_headbuf, headsz, 1);
			Of_SaveFile("/mnt/orig_dec0", (gf **)ass_buf, sz, 1);
		}else{

		   
		   Of_SaveFile("/mnt/orig_dec0", &pkthead[row], headsz, 1);
		   Of_SaveFile("/mnt/orig_dec0", &pkt[row], sz, 1);
		}

	}
#if 0
	/*
	 * move pkts to their final destination
	 * Warning: this function does not update the index[] table to contain
	 * the actual reconstructed packet index.
	 */
	for (row = 0 ; row < k ; row++)
	{
		if (index[row] >= k)
		{
			bcopy (new_pkt[row], pkt[row], sz*sizeof (gf));
			of_free (new_pkt[row]);
		}
	}

	of_free (new_pkt);
#endif
	of_free(ofcb->dec_matrix);
	ofcb->dec_matrix = NULL;
	OF_EXIT_FUNCTION
	return OF_STATUS_OK;

error:
	OF_EXIT_FUNCTION
	return OF_STATUS_FATAL_ERROR;
}
#endif


#ifdef OF_USE_ENCODER
of_status_t	of_rs_2m_encode(of_galois_field_code_cb_t* ofcb,gf *_src[], gf *_fec, int index, int sz)
{
	OF_ENTER_FUNCTION
	gf **src = (gf**) _src;				/* VR */
	gf *fec = (gf*) _fec;				/* VR */
	int i, k = ofcb->nb_source_symbols ;
	gf *p ;

	if (ofcb->m > 8)
		sz /= 2 ;

	if (index < k)
	{
		bcopy (src[index], fec, sz * sizeof(gf));
	}
	else if (index < (ofcb->nb_source_symbols + ofcb->nb_repair_symbols))
	{
		p = & (ofcb->enc_matrix[index*k]);
		bzero (fec, sz * sizeof (gf));
		for (i = 0; i < k ; i++)
		{
			if (p[i] != 0 )
			{
				switch(ofcb->m)
				{
				case 4:
					of_galois_field_2_4_addmul1_compact(fec, src[i], p[i], sz);
					break;
				case 8:
					of_galois_field_2_8_addmul1(fec, src[i], p[i], sz);
					break;						
				}
			}
		}
		return OF_STATUS_OK;
	}
	else
	{
		OF_PRINT_ERROR (("Invalid index %d (max %d)\n", index, ofcb->nb_source_symbols+ofcb->nb_repair_symbols - 1))
	}
	OF_EXIT_FUNCTION
	return OF_STATUS_ERROR;
}
#endif

#endif //OF_USE_GALOIS_FIELD_CODES_UTILS
