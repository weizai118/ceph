
#include "config.h"

#include "objclass/objclass.h"
#include "osd/OSD.h"
#include "osd/ReplicatedPG.h"

#include "common/ClassHandler.h"

static OSD *osd;

void cls_initialize(OSD *_osd)
{
    osd = _osd;
}

void cls_finalize()
{
    osd = NULL;
}


void *cls_alloc(size_t size)
{
  return malloc(size);
}

void cls_free(void *p)
{
  free(p);
}

int cls_register(const char *name, cls_handle_t *handle)
{
  ClassHandler::ClassData *cd;

  cd = osd->class_handler->register_class(name);

  *handle = (cls_handle_t)cd;

  return (cd != NULL);
}

int cls_unregister(cls_handle_t handle)
{
  ClassHandler::ClassData *cd;
  cd = (ClassHandler::ClassData *)handle;

  osd->class_handler->unregister_class(cd);
  return 1;
}

int cls_register_method(cls_handle_t hclass, const char *method,
                        int flags,
                        cls_method_call_t class_call, cls_method_handle_t *handle)
{
  ClassHandler::ClassData *cd;
  cls_method_handle_t hmethod;

  cd = (ClassHandler::ClassData *)hclass;
  hmethod  = (cls_method_handle_t)cd->register_method(method, flags, class_call);
  if (handle)
    *handle = hmethod;
  return (hmethod != NULL);
}

int cls_register_cxx_method(cls_handle_t hclass, const char *method,
                            int flags,
			    cls_method_cxx_call_t class_call, cls_method_handle_t *handle)
{
  ClassHandler::ClassData *cd;
  cls_method_handle_t hmethod;

  cd = (ClassHandler::ClassData *)hclass;
  hmethod  = (cls_method_handle_t)cd->register_cxx_method(method, flags, class_call);
  if (handle)
    *handle = hmethod;
  return (hmethod != NULL);
}

int cls_unregister_method(cls_method_handle_t handle)
{
  ClassHandler::ClassMethod *method = (ClassHandler::ClassMethod *)handle;
  method->unregister();

  return 1;
}

int cls_rdcall(cls_method_context_t hctx, const char *cls, const char *method,
                                 char *indata, int datalen,
                                 char **outdata, int *outdatalen)
{
  ReplicatedPG::OpContext **pctx = (ReplicatedPG::OpContext **)hctx;
  bufferlist odata;
  bufferlist idata;
  vector<OSDOp> nops(1);
  OSDOp& op = nops[0];
  int r;

  op.op.op = CEPH_OSD_OP_RDCALL;
  op.op.class_len = strlen(cls);
  op.op.method_len = strlen(method);
  op.op.indata_len = datalen;
  op.data.append(cls, op.op.class_len);
  op.data.append(method, op.op.method_len);
  op.data.append(indata, datalen);
  r = (*pctx)->pg->do_osd_ops(*pctx, nops, odata);

  *outdata = (char *)malloc(odata.length());
  memcpy(*outdata, odata.c_str(), odata.length());
  *outdatalen = odata.length();

  return r;
}

int cls_getxattr(cls_method_context_t hctx, const char *name,
                                 char **outdata, int *outdatalen)
{
  ReplicatedPG::OpContext **pctx = (ReplicatedPG::OpContext **)hctx;
  bufferlist name_data;
  bufferlist odata;
  vector<OSDOp> nops(1);
  OSDOp& op = nops[0];
  int r;

  op.op.op = CEPH_OSD_OP_GETXATTR;
  op.data.append(name);
  op.op.name_len = strlen(name);
  r = (*pctx)->pg->do_osd_ops(*pctx, nops, odata);

  *outdata = (char *)malloc(odata.length());
  memcpy(*outdata, odata.c_str(), odata.length());
  *outdatalen = odata.length();

  return r;
}

int cls_setxattr(cls_method_context_t hctx, const char *name,
                                 const char *value, int val_len)
{
  ReplicatedPG::OpContext **pctx = (ReplicatedPG::OpContext **)hctx;
  bufferlist name_data;
  bufferlist odata;
  vector<OSDOp> nops(1);
  OSDOp& op = nops[0];
  int r;

  op.op.op = CEPH_OSD_OP_SETXATTR;
  op.data.append(name);
  op.data.append(value);
  op.op.name_len = strlen(name);
  op.op.value_len = val_len;
  r = (*pctx)->pg->do_osd_ops(*pctx, nops, odata);

  return r;
}

int cls_read(cls_method_context_t hctx, int ofs, int len,
                                 char **outdata, int *outdatalen)
{
  ReplicatedPG::OpContext **pctx = (ReplicatedPG::OpContext **)hctx;
  vector<OSDOp> ops(1);
  ops[0].op.op = CEPH_OSD_OP_READ;
  ops[0].op.offset = ofs;
  ops[0].op.length = len;
  bufferlist odata;
  int r = (*pctx)->pg->do_osd_ops(*pctx, ops, odata);

  *outdata = (char *)malloc(odata.length());
  memcpy(*outdata, odata.c_str(), odata.length());
  *outdatalen = odata.length();

  return r;
}

int cls_cxx_read(cls_method_context_t hctx, int ofs, int len, bufferlist *outbl)
{
  ReplicatedPG::OpContext **pctx = (ReplicatedPG::OpContext **)hctx;
  vector<OSDOp> ops(1);
  ops[0].op.op = CEPH_OSD_OP_READ;
  ops[0].op.offset = ofs;
  ops[0].op.length = len;
  return (*pctx)->pg->do_osd_ops(*pctx, ops, *outbl);
}

int cls_cxx_write(cls_method_context_t hctx, int ofs, int len, bufferlist *inbl)
{
  ReplicatedPG::OpContext **pctx = (ReplicatedPG::OpContext **)hctx;
  vector<OSDOp> ops(1);
  ops[0].op.op = CEPH_OSD_OP_WRITE;
  ops[0].op.offset = ofs;
  ops[0].op.length = len;
  ops[0].data = *inbl;
  bufferlist outbl;
  return (*pctx)->pg->do_osd_ops(*pctx, ops, outbl);
}

int cls_cxx_replace(cls_method_context_t hctx, int ofs, int len, bufferlist *inbl)
{
  ReplicatedPG::OpContext **pctx = (ReplicatedPG::OpContext **)hctx;
  vector<OSDOp> ops(2);
  ops[0].op.op = CEPH_OSD_OP_TRUNCATE;
  ops[0].op.offset = 0;
  ops[0].op.length = 0;
  ops[1].op.op = CEPH_OSD_OP_WRITE;
  ops[1].op.offset = ofs;
  ops[1].op.length = len;
  ops[1].data = *inbl;
  bufferlist outbl;
  return (*pctx)->pg->do_osd_ops(*pctx, ops, outbl);
}

