/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "H5VLiod_server.h"

#ifdef H5_HAVE_EFF

/*
 * Programmer:  Mohamad Chaarawi <chaarawi@hdfgroup.gov>
 *              July, 2013
 *
 * Purpose:	The IOD plugin server side map routines.
 */

#define EEXISTS 1

/* temp debug value for faking vl data */
int g_debug_counter = 0;


/*-------------------------------------------------------------------------
 * Function:	H5VL_iod_server_map_create_cb
 *
 * Purpose:	Creates a map as a iod object.
 *
 * Return:	Success:	SUCCEED 
 *		Failure:	Negative
 *
 * Programmer:  Mohamad Chaarawi
 *              February, 2013
 *
 *-------------------------------------------------------------------------
 */
void
H5VL_iod_server_map_create_cb(AXE_engine_t UNUSED axe_engine, 
                              size_t UNUSED num_n_parents, AXE_task_t UNUSED n_parents[], 
                              size_t UNUSED num_s_parents, AXE_task_t UNUSED s_parents[], 
                              void *_op_data)
{
    op_data_t *op_data = (op_data_t *)_op_data;
    map_create_in_t *input = (map_create_in_t *)op_data->input;
    map_create_out_t output;
    iod_handle_t coh = input->coh; /* the container handle */
    iod_handle_t loc_handle = input->loc_oh; /* The handle for current object - could be undefined */
    iod_obj_id_t loc_id = input->loc_id; /* The ID of the current location object */
    iod_obj_id_t map_id = input->map_id; /* The ID of the map that needs to be created */
    iod_obj_id_t mdkv_id = input->mdkv_id; /* The ID of the metadata KV to be created */
    iod_obj_id_t attr_id = input->attrkv_id; /* The ID of the attirbute KV to be created */
    const char *name = input->name; /* path relative to loc_id and loc_oh  */
    hid_t keytype = input->keytype_id;
    hid_t valtype = input->valtype_id;
    iod_trans_id_t wtid = input->trans_num;
    iod_trans_id_t rtid = input->rcxt_num;
    uint32_t cs_scope = input->cs_scope;
    iod_handle_t map_oh, cur_oh, mdkv_oh;
    iod_obj_id_t cur_id;
    char *last_comp; /* the name of the group obtained from traversal function */
    hid_t mcpl_id;
    scratch_pad sp;
    iod_ret_t ret;
    hbool_t collective = FALSE; /* MSC - change when we allow for collective */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

#if H5VL_IOD_DEBUG
    fprintf(stderr, "Start map create %s\n", name);
#endif

    /* the traversal will retrieve the location where the map needs
       to be created. The traversal will fail if an intermediate group
       does not exist. */
    if(H5VL_iod_server_traverse(coh, loc_id, loc_handle, name, rtid, FALSE, 
                                &last_comp, &cur_id, &cur_oh) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOSPACE, FAIL, "can't traverse path");

    /* create the map */
    ret = iod_obj_create(coh, wtid, NULL, IOD_OBJ_KV, 
                         NULL, NULL, &map_id, NULL);
    if(collective && (0 == ret || EEXISTS == ret)) {
        /* map has been created by another process, open it */
        if (iod_obj_open_write(coh, map_id, NULL, &map_oh, NULL) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't open Map");
    }
    else if(!collective && 0 != ret) {
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't create Map");
    }

    /* for the process that succeeded in creating the map, create
       the scratch pad for it too */
    if(0 == ret) {
        /* create the metadata KV object for the map */
        if(iod_obj_create(coh, wtid, NULL, IOD_OBJ_KV, 
                          NULL, NULL, &mdkv_id, NULL) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't create metadata KV object");

        /* create the attribute KV object for the root group */
        if(iod_obj_create(coh, wtid, NULL, IOD_OBJ_KV, 
                          NULL, NULL, &attr_id, NULL) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't create metadata KV object");

        /* set values for the scratch pad object */
        sp[0] = mdkv_id;
        sp[1] = attr_id;
        sp[2] = IOD_ID_UNDEFINED;
        sp[3] = IOD_ID_UNDEFINED;

        /* set scratch pad in map */
        if(cs_scope & H5_CHECKSUM_IOD) {
            iod_checksum_t sp_cs;

            sp_cs = H5checksum(&sp, sizeof(sp), NULL);
            if (iod_obj_set_scratch(map_oh, wtid, &sp, &sp_cs, NULL) < 0)
                HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't set scratch pad");
        }
        else {
            if (iod_obj_set_scratch(map_oh, wtid, &sp, NULL, NULL) < 0)
                HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't set scratch pad");
        }

        /* Open Metadata KV object for write */
        if (iod_obj_open_write(coh, mdkv_id, NULL, &mdkv_oh, NULL) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't create scratch pad");

        if(H5P_DEFAULT == input->mcpl_id)
            input->mcpl_id = H5Pcopy(H5P_GROUP_CREATE_DEFAULT);
        mcpl_id = input->mcpl_id;

        /* insert plist metadata */
        if(H5VL_iod_insert_plist(mdkv_oh, wtid, mcpl_id, 
                                 NULL, NULL, NULL) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't insert KV value");

        /* insert link count metadata */
        if(H5VL_iod_insert_link_count(mdkv_oh, wtid, (uint64_t)1, 
                                      NULL, NULL, NULL) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't insert KV value");

        /* insert object type metadata */
        if(H5VL_iod_insert_object_type(mdkv_oh, wtid, H5I_MAP, 
                                       NULL, NULL, NULL) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't insert KV value");

        /* MSC - insert Key datatype metadata */
        /* MSC - insert Value datatype metadata */

        /* close MD KV object */
        if(iod_obj_close(mdkv_oh, NULL, NULL))
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't close object");

        /* add link in parent group to current object */
        if(H5VL_iod_insert_new_link(cur_oh, wtid, last_comp, 
                                    H5L_TYPE_HARD, &map_id, NULL, NULL, NULL) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't insert KV value");
    } /* end if */

    /* close parent group if it is not the location we started the
       traversal into */
    if(loc_handle.cookie != cur_oh.cookie) {
        iod_obj_close(cur_oh, NULL, NULL);
    }

#if H5VL_IOD_DEBUG
    fprintf(stderr, "Done with map create, sending response to client\n");
#endif

    /* return the object handle for the map to the client */
    output.iod_oh = map_oh;
    HG_Handler_start_output(op_data->hg_handle, &output);

done:
    /* return an UNDEFINED oh to the client if the operation failed */
    if(ret_value < 0) {
        fprintf(stderr, "Failed Map Create\n");
        output.iod_oh.cookie = IOD_OH_UNDEFINED;
        HG_Handler_start_output(op_data->hg_handle, &output);
    }

    last_comp = (char *)H5MM_xfree(last_comp);
    input = (map_create_in_t *)H5MM_xfree(input);
    op_data = (op_data_t *)H5MM_xfree(op_data);

    FUNC_LEAVE_NOAPI_VOID
} /* end H5VL_iod_server_map_create_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5VL_iod_server_map_open_cb
 *
 * Purpose:	Opens a map as a iod object.
 *
 * Return:	Success:	SUCCEED 
 *		Failure:	Negative
 *
 * Programmer:  Mohamad Chaarawi
 *              February, 2013
 *
 *-------------------------------------------------------------------------
 */
void
H5VL_iod_server_map_open_cb(AXE_engine_t UNUSED axe_engine, 
                            size_t UNUSED num_n_parents, AXE_task_t UNUSED n_parents[], 
                            size_t UNUSED num_s_parents, AXE_task_t UNUSED s_parents[], 
                            void *_op_data)
{
    op_data_t *op_data = (op_data_t *)_op_data;
    map_open_in_t *input = (map_open_in_t *)op_data->input;
    map_open_out_t output;
    iod_handle_t coh = input->coh;
    iod_handle_t loc_handle = input->loc_oh;
    iod_obj_id_t loc_id = input->loc_id;
    const char *name = input->name;
    iod_trans_id_t rtid = input->rcxt_num;
    uint32_t cs_scope = input->cs_scope;
    iod_obj_id_t map_id; /* The ID of the map that needs to be opened */
    iod_handle_t map_oh, mdkv_oh;
    scratch_pad sp;
    iod_checksum_t sp_cs = 0;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

#if H5VL_IOD_DEBUG
    fprintf(stderr, "Start map open %s with Loc ID %llu\n", name, loc_id);
#endif

    output.keytype_id = -1;
    output.valtype_id = -1;

    /* Traverse Path and open map */
    if(H5VL_iod_server_open_path(coh, loc_id, loc_handle, name, rtid, &map_id, &map_oh) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOSPACE, FAIL, "can't open object");

    /* get scratch pad of map */
    if(iod_obj_get_scratch(map_oh, rtid, &sp, &sp_cs, NULL) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "can't get scratch pad for object");

    if(sp_cs && (cs_scope & H5_CHECKSUM_IOD)) {
        /* verify scratch pad integrity */
        if(H5VL_iod_verify_scratch_pad(sp, sp_cs) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "Scratch Pad failed integrity check");
    }

    /* open the metadata scratch pad */
    if (iod_obj_open_read(coh, sp[0], NULL /*hints*/, &mdkv_oh, NULL) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "can't open scratch pad");

    /* MSC - retrieve metadata - need IOD*/
#if 0
    if(H5VL_iod_get_metadata(mdkv_oh, rtid, H5VL_IOD_PLIST, 
                             H5VL_IOD_KEY_OBJ_CPL, NULL, NULL, &output.mcpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "failed to retrieve gcpl");

    if(H5VL_iod_get_metadata(mdkv_oh, rtid, H5VL_IOD_LINK_COUNT, 
                             H5VL_IOD_KEY_OBJ_LINK_COUNT,
                             NULL, NULL, &output.link_count) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "failed to retrieve link count");
#endif

    /* close the metadata scratch pad */
    if(iod_obj_close(mdkv_oh, NULL, NULL))
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't close meta data KV handle");

    /* MSC - fake stuff for now*/
    map_oh.cookie = 1;
    output.keytype_id = H5Tcopy(H5T_NATIVE_INT);
    output.valtype_id = H5Tcopy(H5T_NATIVE_INT);
    output.mcpl_id = H5P_GROUP_CREATE_DEFAULT;

    output.iod_id = map_id;
    output.mdkv_id = sp[0];
    output.attrkv_id = sp[1];
    output.iod_oh = map_oh;

#if H5VL_IOD_DEBUG
    fprintf(stderr, "Done with map open, sending response to client\n");
#endif

    HG_Handler_start_output(op_data->hg_handle, &output);

done:
    if(ret_value < 0) {
        output.iod_oh.cookie = IOD_OH_UNDEFINED;
        output.iod_id = IOD_ID_UNDEFINED;
        HG_Handler_start_output(op_data->hg_handle, &output);
    }

    if(output.keytype_id)
        H5Tclose(output.keytype_id);
    if(output.valtype_id)
        H5Tclose(output.valtype_id);

    input = (map_open_in_t *)H5MM_xfree(input);
    op_data = (op_data_t *)H5MM_xfree(op_data);

    FUNC_LEAVE_NOAPI_VOID
} /* end H5VL_iod_server_map_open_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5VL_iod_server_map_set_cb
 *
 * Purpose:	Insert/Set a KV pair in map object
 *
 * Return:	Success:	SUCCEED 
 *		Failure:	Negative
 *
 * Programmer:  Mohamad Chaarawi
 *              July, 2013
 *
 *-------------------------------------------------------------------------
 */
void
H5VL_iod_server_map_set_cb(AXE_engine_t UNUSED axe_engine, 
                           size_t UNUSED num_n_parents, AXE_task_t UNUSED n_parents[], 
                           size_t UNUSED num_s_parents, AXE_task_t UNUSED s_parents[], 
                           void *_op_data)
{
    op_data_t *op_data = (op_data_t *)_op_data;
    map_set_in_t *input = (map_set_in_t *)op_data->input;
    iod_handle_t coh = input->coh;
    iod_handle_t iod_oh = input->iod_oh;
    iod_obj_id_t iod_id = input->iod_id; 
    hid_t key_memtype_id = input->key_memtype_id;
    hid_t val_memtype_id = input->val_memtype_id;
    hid_t key_maptype_id = input->key_maptype_id;
    hid_t val_maptype_id = input->val_maptype_id;
    binary_buf_t key = input->key;
    hg_bulk_t value_handle = input->val_handle; /* bulk handle for data */
    uint32_t value_cs = input->val_checksum; /* checksum recieved for data */
    hid_t dxpl_id = input->dxpl_id;
    iod_trans_id_t wtid = input->trans_num;
    iod_trans_id_t rtid = input->rcxt_num;
    uint32_t cs_scope = input->cs_scope;
    na_addr_t source = HG_Handler_get_addr(op_data->hg_handle); /* source address to pull data from */
    hg_bulk_block_t bulk_block_handle; /* HG block handle */
    hg_bulk_request_t bulk_request; /* HG request */
    size_t key_size, val_size, new_val_size;
    void *val_buf = NULL;
    iod_kv_t kv;
    uint32_t raw_cs_scope;
    hbool_t opened_locally = FALSE;
    hbool_t val_is_vl_data = FALSE, key_is_vl_data = FALSE;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

#if H5VL_IOD_DEBUG
    fprintf(stderr, "Start map set\n");
#endif

    /* open the map if we don't have the handle yet */
    if(iod_oh.cookie == IOD_OH_UNDEFINED) {
        if (iod_obj_open_write(coh, iod_id, NULL /*hints*/, &iod_oh, NULL) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't open current group");
        opened_locally = TRUE;
    }

    if(H5P_DEFAULT == input->dxpl_id)
        input->dxpl_id = H5Pcopy(H5P_DATASET_XFER_DEFAULT);
    dxpl_id = input->dxpl_id;

    /* retrieve size of incoming bulk data */
    val_size = HG_Bulk_handle_get_size(value_handle);

    /* allocate buffer to hold data */
    if(NULL == (val_buf = malloc(val_size)))
        HGOTO_ERROR(H5E_SYM, H5E_NOSPACE, FAIL, "can't allocate read buffer");

    /* create a Mercury block handle for transfer */
    HG_Bulk_block_handle_create(val_buf, val_size, HG_BULK_READWRITE, &bulk_block_handle);

    /* Write bulk data here and wait for the data to be there  */
    if(HG_SUCCESS != HG_Bulk_read_all(source, value_handle, bulk_block_handle, &bulk_request))
        HGOTO_ERROR(H5E_SYM, H5E_WRITEERROR, FAIL, "can't get data from function shipper");
    /* wait for it to complete */
    if(HG_SUCCESS != HG_Bulk_wait(bulk_request, HG_MAX_IDLE_TIME, HG_STATUS_IGNORE))
        HGOTO_ERROR(H5E_SYM, H5E_WRITEERROR, FAIL, "can't get data from function shipper");

    /* free the bds block handle */
    if(HG_SUCCESS != HG_Bulk_block_handle_free(bulk_block_handle))
        HGOTO_ERROR(H5E_SYM, H5E_WRITEERROR, FAIL, "can't free bds block handle");

    /* get the scope for data integrity checks for raw data */
    if(H5Pget_rawdata_integrity_scope(dxpl_id, &raw_cs_scope) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get scope for data integrity checks");

    /* verify data if transfer flag is set */
    if(raw_cs_scope & H5_CHECKSUM_TRANSFER) {
        uint32_t data_cs;

        data_cs = H5checksum(val_buf, val_size, NULL);
        if(value_cs != data_cs) {
            fprintf(stderr, "Errrr.. Network transfer Data corruption. expecting %u, got %u\n",
                    value_cs, data_cs);
            ret_value = FAIL;
            goto done;
        }
    }
#if H5VL_IOD_DEBUG
    else {
        fprintf(stderr, "NO TRANSFER DATA INTEGRITY CHECKS ON RAW DATA\n");
    }
#endif

    /* adjust buffers for datatype conversion */
    if(H5VL__iod_server_adjust_buffer(key_memtype_id, key_maptype_id, 1, dxpl_id, 
                                      key.buf_size, &key.buf, &key_is_vl_data, &key_size) < 0) 
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "data type conversion failed");

    if(H5VL__iod_server_adjust_buffer(val_memtype_id, val_maptype_id, 1, dxpl_id, 
                                      val_size, &val_buf, &val_is_vl_data, &new_val_size) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "data type conversion failed");

    /* MSC - fake debugging */
    if(val_is_vl_data) {
        H5T_class_t class;
        size_t seq_len = val_size, u;
        uint8_t *buf_ptr = (uint8_t *)val_buf;

        class = H5Tget_class(val_memtype_id);

        if(H5T_STRING == class)
            fprintf(stderr, "String Length %zu: %s\n", seq_len, (char *)buf_ptr);
        else if(H5T_VLEN == class) {
            int *ptr = (int *)buf_ptr;

            fprintf(stderr, "Sequence Count %zu: ", seq_len);
            for(u=0 ; u<seq_len/sizeof(int) ; ++u)
                fprintf(stderr, "%d ", ptr[u]);
            fprintf(stderr, "\n");
        }
    }
    else {
        fprintf(stderr, "Map Set value = %d; size = %zu\n", *((int *)val_buf), val_size);
    }

    if(!key_is_vl_data) {
        /* convert data if needed */
        if(H5Tconvert(key_memtype_id, key_maptype_id, 1, key.buf, NULL, dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "data type conversion failed");
    }
    if(!val_is_vl_data) {
        if(H5Tconvert(val_memtype_id, val_maptype_id, 1, val_buf, NULL, dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "data type conversion failed");
    }

    /* MSC - do IOD checksum - can't now */
    kv.key = key.buf;
    kv.value = val_buf;
    kv.value_len = (iod_size_t)new_val_size;
    /* insert kv pair into MAP */
    if (iod_kv_set(iod_oh, wtid, NULL, &kv, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't set KV pair in parent");

done:

    if(HG_SUCCESS != HG_Handler_start_output(op_data->hg_handle, &ret_value))
        HDONE_ERROR(H5E_SYM, H5E_WRITEERROR, FAIL, "can't send result of write to client");

    input = (map_set_in_t *)H5MM_xfree(input);
    op_data = (op_data_t *)H5MM_xfree(op_data);

    if(val_buf)
        free(val_buf);

    /* close the map if we opened it in this routine */
    if(opened_locally) {
        if(iod_obj_close(iod_oh, NULL, NULL))
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't close Array object");
    }

#if H5VL_IOD_DEBUG 
    fprintf(stderr, "Done with map set, sent %d response to client\n", ret_value);
#endif

    FUNC_LEAVE_NOAPI_VOID
} /* end H5VL_iod_server_map_set_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5VL_iod_server_map_get_cb
 *
 * Purpose:	Get a KV pair in map object
 *
 * Return:	Success:	SUCCEED 
 *		Failure:	Negative
 *
 * Programmer:  Mohamad Chaarawi
 *              July, 2013
 *
 *-------------------------------------------------------------------------
 */
void
H5VL_iod_server_map_get_cb(AXE_engine_t UNUSED axe_engine, 
                           size_t UNUSED num_n_parents, AXE_task_t UNUSED n_parents[], 
                           size_t UNUSED num_s_parents, AXE_task_t UNUSED s_parents[], 
                           void *_op_data)
{
    op_data_t *op_data = (op_data_t *)_op_data;
    map_get_in_t *input = (map_get_in_t *)op_data->input;
    map_get_out_t output;
    iod_handle_t coh = input->coh;
    iod_handle_t iod_oh = input->iod_oh;
    iod_obj_id_t iod_id = input->iod_id; 
    hid_t key_memtype_id = input->key_memtype_id;
    hid_t val_memtype_id = input->val_memtype_id;
    hid_t key_maptype_id = input->key_maptype_id;
    hid_t val_maptype_id = input->val_maptype_id;
    binary_buf_t key = input->key;
    hid_t dxpl_id = input->dxpl_id;
    iod_trans_id_t rtid = input->rcxt_num;
    uint32_t cs_scope = input->cs_scope;
    hbool_t val_is_vl = input->val_is_vl;
    size_t client_val_buf_size = input->val_size;
    hg_bulk_t value_handle = input->val_handle; /* bulk handle for data */
    uint32_t raw_cs_scope;
    hbool_t key_is_vl = FALSE;
    size_t key_size, val_size;
    iod_size_t src_size;
    void *val_buf = NULL;
    hg_bulk_block_t bulk_block_handle; /* HG block handle */
    hg_bulk_request_t bulk_request; /* HG request */
    na_addr_t dest = HG_Handler_get_addr(op_data->hg_handle); /* destination address to push data to */
    hbool_t opened_locally = FALSE;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

#if H5VL_IOD_DEBUG
    fprintf(stderr, "Start map get \n");
#endif

    /* open the map if we don't have the handle yet */
    if(iod_oh.cookie == IOD_OH_UNDEFINED) {
        if (iod_obj_open_write(coh, iod_id, NULL /*hints*/, &iod_oh, NULL) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't open current group");
        opened_locally = TRUE;
    }

    if(H5P_DEFAULT == input->dxpl_id)
        input->dxpl_id = H5Pcopy(H5P_DATASET_XFER_DEFAULT);
    dxpl_id = input->dxpl_id;

    /* get the scope for data integrity checks for raw data */
    if(H5Pget_rawdata_integrity_scope(dxpl_id, &raw_cs_scope) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get scope for data integrity checks");

    /* adjust buffers for datatype conversion */
    if(H5VL__iod_server_adjust_buffer(key_memtype_id, key_maptype_id, 1, dxpl_id, 
                                      key.buf_size, &key.buf, &key_is_vl, &key_size) < 0) 
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "data type conversion failed");

    if(iod_kv_get_value(iod_oh, rtid, key.buf, NULL, 
                        &src_size, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't retrieve value from parent KV store");

    if(val_is_vl) {
        H5T_class_t class;

        if(iod_kv_get_value(iod_oh, rtid, key.buf, val_buf, 
                            &src_size, NULL, NULL) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't retrieve value from parent KV store");
        class = H5Tget_class(val_memtype_id);

        /* MSC - fake something */
        val_buf = NULL;
        if(!client_val_buf_size) {
            g_debug_counter ++;
            if(g_debug_counter == 6) g_debug_counter=1;
        }

        if(H5T_VLEN == class) {
            int n=0, k=0, j=0, increment=4;
            int *ptr;

            src_size = g_debug_counter*increment*sizeof(int);

            if(NULL == (val_buf = malloc((size_t)src_size)))
                HGOTO_ERROR(H5E_SYM, H5E_NOSPACE, FAIL, "can't allocate buffer");

            ptr = (int *)val_buf;

            for(j = 0; j < g_debug_counter*increment; j++)
                ptr[k++] = n ++;
        }
        else if (H5T_STRING == class) {
            uint8_t *ptr;
            size_t temp_size;

            temp_size = 94;
            if(NULL == (val_buf = malloc(temp_size)))
                HGOTO_ERROR(H5E_SYM, H5E_NOSPACE, FAIL, "can't allocate buffer");

            ptr = (uint8_t *)val_buf;

            switch(g_debug_counter) {
            case 1:
                src_size = 93;
                strcpy((char *)ptr, "Four score and seven years ago our forefathers brought forth on this continent a new nation,");
                ptr += src_size;
                break;
            case 2:
                src_size = 86;
                strcpy((char *)ptr, "conceived in liberty and dedicated to the proposition that all men are created equal.");
                ptr += src_size;
                break;
            case 3:
                src_size = 41;
                strcpy((char *)ptr, "Now we are engaged in a great civil war,");
                ptr += src_size;
                break;
            case 4:
                src_size = 89;
                strcpy((char *)ptr, "testing whether that nation or any nation so conceived and so dedicated can long endure.");
                ptr += src_size;
                break;
            case 5:
                src_size = 26;
                strcpy((char *)ptr, "President Abraham Lincoln");
                ptr += src_size;
                break;
            default:
                HDassert(0 && "should not be here!!!");
            }
        }

        output.ret = ret_value;
        output.val_size = src_size;
        if(raw_cs_scope) {
            /* calculate a checksum for the data to be sent */
            output.val_cs = H5checksum(val_buf, (size_t)src_size, NULL);
        }
#if H5VL_IOD_DEBUG
        else {
            fprintf(stderr, "NO TRANSFER DATA INTEGRITY CHECKS ON RAW DATA\n");
        }
#endif
        if(client_val_buf_size) {
            /* Create a new block handle to write the data */
            HG_Bulk_block_handle_create(val_buf, (size_t)src_size, HG_BULK_READ_ONLY, &bulk_block_handle);

            /* Write bulk data here and wait for the data to be there  */
            if(HG_SUCCESS != HG_Bulk_write_all(dest, value_handle, bulk_block_handle, &bulk_request))
                HGOTO_ERROR(H5E_SYM, H5E_READERROR, FAIL, "can't read from array object");
            /* wait for it to complete */
            if(HG_SUCCESS != HG_Bulk_wait(bulk_request, HG_MAX_IDLE_TIME, HG_STATUS_IGNORE))
                HGOTO_ERROR(H5E_SYM, H5E_READERROR, FAIL, "can't read from array object");

            /* free block handle */
            if(HG_SUCCESS != HG_Bulk_block_handle_free(bulk_block_handle))
                HGOTO_ERROR(H5E_SYM, H5E_READERROR, FAIL, "can't free bds block handle");
        }
    }
    else {
        /* retrieve size of bulk data asked for to be read */
        src_size = HG_Bulk_handle_get_size(value_handle);

        if(NULL == (val_buf = malloc((size_t)src_size)))
            HGOTO_ERROR(H5E_SYM, H5E_NOSPACE, FAIL, "can't allocate buffer");

        if(iod_kv_get_value(iod_oh, rtid, key.buf, val_buf, 
                            &src_size, NULL, NULL) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't retrieve value from parent KV store");

        /* adjust buffers for datatype conversion */
        if(H5VL__iod_server_adjust_buffer(val_memtype_id, val_maptype_id, 1, dxpl_id, 
                                          (size_t)src_size, &val_buf, &val_is_vl, &val_size) < 0) 
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "data type conversion failed");

        /* do data conversion */
        if(H5Tconvert(val_maptype_id, val_memtype_id, 1, val_buf, NULL, dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "data type conversion failed");

        /* MSC - fake something */
        *((int *)val_buf) = 1024;

        if(raw_cs_scope) {
            /* calculate a checksum for the data to be sent */
            output.val_cs = H5checksum(val_buf, val_size, NULL);
        }
#if H5VL_IOD_DEBUG
        else {
            fprintf(stderr, "NO TRANSFER DATA INTEGRITY CHECKS ON RAW DATA\n");
        }
#endif

        output.val_size = val_size;
        output.ret = ret_value;

        /* Create a new block handle to write the data */
        HG_Bulk_block_handle_create(val_buf, val_size, HG_BULK_READ_ONLY, &bulk_block_handle);

        /* Write bulk data here and wait for the data to be there  */
        if(HG_SUCCESS != HG_Bulk_write_all(dest, value_handle, bulk_block_handle, &bulk_request))
            HGOTO_ERROR(H5E_SYM, H5E_READERROR, FAIL, "can't read from array object");
        /* wait for it to complete */
        if(HG_SUCCESS != HG_Bulk_wait(bulk_request, HG_MAX_IDLE_TIME, HG_STATUS_IGNORE))
            HGOTO_ERROR(H5E_SYM, H5E_READERROR, FAIL, "can't read from array object");

        /* free block handle */
        if(HG_SUCCESS != HG_Bulk_block_handle_free(bulk_block_handle))
            HGOTO_ERROR(H5E_SYM, H5E_READERROR, FAIL, "can't free bds block handle");
    }

#if H5VL_IOD_DEBUG 
    fprintf(stderr, "Done with map get, sending %d response to client\n", ret_value);
#endif

    if(HG_SUCCESS != HG_Handler_start_output(op_data->hg_handle, &output))
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't send result of map get");

done:

    if(ret_value < 0) {
        output.ret = FAIL;
        output.val_size = 0;
        output.val_cs = 0;
        if(HG_SUCCESS != HG_Handler_start_output(op_data->hg_handle, &output))
            HDONE_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't send result of map get");
    }

    if(val_buf) 
        free(val_buf);

    input = (map_get_in_t *)H5MM_xfree(input);
    op_data = (op_data_t *)H5MM_xfree(op_data);

    /* close the map if we opened it in this routine */
    if(opened_locally) {
        if(iod_obj_close(iod_oh, NULL, NULL))
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't close Array object");
    }

    FUNC_LEAVE_NOAPI_VOID
} /* end H5VL_iod_server_map_get_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5VL_iod_server_map_get_count_cb
 *
 * Purpose:	Get number of KV pairs in map object
 *
 * Return:	Success:	SUCCEED 
 *		Failure:	Negative
 *
 * Programmer:  Mohamad Chaarawi
 *              July, 2013
 *
 *-------------------------------------------------------------------------
 */
void
H5VL_iod_server_map_get_count_cb(AXE_engine_t UNUSED axe_engine, 
                           size_t UNUSED num_n_parents, AXE_task_t UNUSED n_parents[], 
                           size_t UNUSED num_s_parents, AXE_task_t UNUSED s_parents[], 
                           void *_op_data)
{
    op_data_t *op_data = (op_data_t *)_op_data;
    map_get_count_in_t *input = (map_get_count_in_t *)op_data->input;
    iod_handle_t coh = input->coh;
    iod_handle_t iod_oh = input->iod_oh;
    iod_obj_id_t iod_id = input->iod_id;
    iod_trans_id_t rtid = input->rcxt_num;
    uint32_t cs_scope = input->cs_scope;
    iod_size_t num;
    hbool_t opened_locally = FALSE;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

#if H5VL_IOD_DEBUG
    fprintf(stderr, "Start map get_count \n");
#endif

    /* open the map if we don't have the handle yet */
    if(iod_oh.cookie == IOD_OH_UNDEFINED) {
        if (iod_obj_open_write(coh, iod_id, NULL /*hints*/, &iod_oh, NULL) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't open current group");
        opened_locally = TRUE;
    }

    if(iod_kv_get_num(iod_oh, rtid, &num, NULL) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't retrieve Number of KV pairs in MAP");

    /* MSC - fake something for now */
    num = 3;

#if H5VL_IOD_DEBUG 
    fprintf(stderr, "Done with map get_count, sending %d response to client\n", ret_value);
#endif

    if(HG_SUCCESS != HG_Handler_start_output(op_data->hg_handle, &num))
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't send result of map get");

done:

    if(ret_value < 0) {
        num = IOD_COUNT_UNDEFINED;
        if(HG_SUCCESS != HG_Handler_start_output(op_data->hg_handle, &num))
            HDONE_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't send result of map get_count");
    }

    input = (map_get_count_in_t *)H5MM_xfree(input);
    op_data = (op_data_t *)H5MM_xfree(op_data);

    /* close the map if we opened it in this routine */
    if(opened_locally) {
        if(iod_obj_close(iod_oh, NULL, NULL))
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't close Array object");
    }

    FUNC_LEAVE_NOAPI_VOID
} /* end H5VL_iod_server_map_get_count_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5VL_iod_server_map_exists_cb
 *
 * Purpose:	check if a key Exists in map object
 *
 * Return:	Success:	SUCCEED 
 *		Failure:	Negative
 *
 * Programmer:  Mohamad Chaarawi
 *              July, 2013
 *
 *-------------------------------------------------------------------------
 */
void
H5VL_iod_server_map_exists_cb(AXE_engine_t UNUSED axe_engine, 
                              size_t UNUSED num_n_parents, AXE_task_t UNUSED n_parents[], 
                              size_t UNUSED num_s_parents, AXE_task_t UNUSED s_parents[], 
                              void *_op_data)
{
    op_data_t *op_data = (op_data_t *)_op_data;
    map_op_in_t *input = (map_op_in_t *)op_data->input;
    iod_handle_t coh = input->coh;
    iod_handle_t iod_oh = input->iod_oh;
    iod_obj_id_t iod_id = input->iod_id; 
    hid_t key_memtype_id = input->key_memtype_id;
    hid_t key_maptype_id = input->key_maptype_id;
    binary_buf_t key = input->key;
    iod_trans_id_t rtid = input->rcxt_num;
    uint32_t cs_scope = input->cs_scope;
    size_t key_size;
    iod_size_t val_size;
    hbool_t opened_locally = FALSE;
    htri_t exists;
    hbool_t is_vl_data = FALSE;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

#if H5VL_IOD_DEBUG
    fprintf(stderr, "Start map exists \n");
#endif

    /* open the map if we don't have the handle yet */
    if(iod_oh.cookie == IOD_OH_UNDEFINED) {
        if (iod_obj_open_write(coh, iod_id, NULL /*hints*/, &iod_oh, NULL) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't open current group");
        opened_locally = TRUE;
    }

    /* adjust buffers for datatype conversion */
    if(H5VL__iod_server_adjust_buffer(key_memtype_id, key_maptype_id, 1, H5P_DEFAULT, 
                                      key.buf_size, &key.buf, &is_vl_data, &key_size) < 0) 
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "data type conversion failed");

    /* determine if the Key exists by querying its value size */
    if(iod_kv_get_value(iod_oh, rtid, key.buf, NULL, 
                        &val_size, NULL, NULL) < 0)
        exists = FALSE;
    else
        exists = TRUE;


#if H5VL_IOD_DEBUG 
    fprintf(stderr, "Done with map exists, sending %d response to client\n", ret_value);
#endif

    if(HG_SUCCESS != HG_Handler_start_output(op_data->hg_handle, &exists))
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't send result of map get");

done:

    if(ret_value < 0) {
        exists = -1; 
        if(HG_SUCCESS != HG_Handler_start_output(op_data->hg_handle, &exists))
            HDONE_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't send result of map exists");
    }

    input = (map_op_in_t *)H5MM_xfree(input);
    op_data = (op_data_t *)H5MM_xfree(op_data);

    /* close the map if we opened it in this routine */
    if(opened_locally) {
        if(iod_obj_close(iod_oh, NULL, NULL))
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't close Array object");
    }

    FUNC_LEAVE_NOAPI_VOID
} /* end H5VL_iod_server_map_exists_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5VL_iod_server_map_delete_cb
 *
 * Purpose:	Delete a KV pair in map object
 *
 * Programmer:  Mohamad Chaarawi
 *              July, 2013
 *
 *-------------------------------------------------------------------------
 */
void
H5VL_iod_server_map_delete_cb(AXE_engine_t UNUSED axe_engine, 
                              size_t UNUSED num_n_parents, AXE_task_t UNUSED n_parents[], 
                              size_t UNUSED num_s_parents, AXE_task_t UNUSED s_parents[], 
                              void *_op_data)
{
    op_data_t *op_data = (op_data_t *)_op_data;
    map_op_in_t *input = (map_op_in_t *)op_data->input;
    iod_handle_t coh = input->coh;
    iod_handle_t iod_oh = input->iod_oh;
    iod_obj_id_t iod_id = input->iod_id; 
    hid_t key_memtype_id = input->key_memtype_id;
    hid_t key_maptype_id = input->key_maptype_id;
    binary_buf_t key = input->key;
    iod_trans_id_t wtid = input->trans_num;
    iod_trans_id_t rtid = input->rcxt_num;
    uint32_t cs_scope = input->cs_scope;
    size_t key_size;
    iod_kv_t kv;
    iod_kv_params_t kvs;
    hbool_t opened_locally = FALSE;
    hbool_t is_vl_data = FALSE;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

#if H5VL_IOD_DEBUG
    fprintf(stderr, "Start map delete \n");
#endif

    /* open the map if we don't have the handle yet */
    if(iod_oh.cookie == IOD_OH_UNDEFINED) {
        if (iod_obj_open_write(coh, iod_id, NULL /*hints*/, &iod_oh, NULL) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't open current group");
        opened_locally = TRUE;
    }

    /* adjust buffers for datatype conversion */
    if(H5VL__iod_server_adjust_buffer(key_memtype_id, key_maptype_id, 1, H5P_DEFAULT, 
                                      key.buf_size, &key.buf, &is_vl_data, &key_size) < 0) 
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "data type conversion failed");

    kv.key = key.buf;
    kvs.kv = &kv;

    if(iod_kv_unlink_keys(iod_oh, wtid, NULL, (iod_size_t)1, &kvs, NULL) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTDEC, FAIL, "Unable to unlink KV pair");

done:

#if H5VL_IOD_DEBUG 
    fprintf(stderr, "Done with map delete, sending %d response to client\n", ret_value);
#endif

    if(HG_SUCCESS != HG_Handler_start_output(op_data->hg_handle, &ret_value))
        HDONE_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't send result of map delete");

    input = (map_op_in_t *)H5MM_xfree(input);
    op_data = (op_data_t *)H5MM_xfree(op_data);

    /* close the map if we opened it in this routine */
    if(opened_locally) {
        if(iod_obj_close(iod_oh, NULL, NULL))
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't close Array object");
    }

    FUNC_LEAVE_NOAPI_VOID
} /* end H5VL_iod_server_map_delete_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5VL_iod_server_map_close_cb
 *
 * Purpose:	Closes iod HDF5 map.
 *
 * Return:	Success:	SUCCEED 
 *		Failure:	Negative
 *
 * Programmer:  Mohamad Chaarawi
 *              January, 2013
 *
 *-------------------------------------------------------------------------
 */
void
H5VL_iod_server_map_close_cb(AXE_engine_t UNUSED axe_engine, 
                             size_t UNUSED num_n_parents, AXE_task_t UNUSED n_parents[], 
                             size_t UNUSED num_s_parents, AXE_task_t UNUSED s_parents[], 
                             void *_op_data)
{
    op_data_t *op_data = (op_data_t *)_op_data;
    map_close_in_t *input = (map_close_in_t *)op_data->input;
    iod_handle_t iod_oh = input->iod_oh;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

#if H5VL_IOD_DEBUG
    fprintf(stderr, "Start map close\n");
#endif

    if(iod_oh.cookie != IOD_OH_UNDEFINED) {
        if((ret_value = iod_obj_close(iod_oh, NULL, NULL)) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't close object");
    }
    else {
        /* MSC - need a way to kill object handle for this map */
        fprintf(stderr, "I do not have the OH of this map to close it\n");
    }
done:
#if H5VL_IOD_DEBUG
    fprintf(stderr, "Done with map close, sending response to client\n");
#endif

    HG_Handler_start_output(op_data->hg_handle, &ret_value);

    input = (map_close_in_t *)H5MM_xfree(input);
    op_data = (op_data_t *)H5MM_xfree(op_data);

    FUNC_LEAVE_NOAPI_VOID
} /* end H5VL_iod_server_map_close_cb() */

#endif /* H5_HAVE_EFF */