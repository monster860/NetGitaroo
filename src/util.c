#include <stdio.h>
#include <kernel.h>
#include <sifrpc.h>
// Various stuff from ps2sdk that cannot be found in the original ELF,
// and won't break horribly if duplicated

#undef putchar
int putsn(const char *str)
{
	int res;

	for (res = 0; *str; res++, str++)
	    putchar(*str);

	return res;
}

int puts(const char *str)
{
	int res;

	res = putsn(str);

	putchar('\r');
	putchar('\n');
	return res + 2;
}

int DIntr()
{
	int eie, res;

	asm volatile ("mfc0\t%0, $12" : "=r" (eie));
	eie &= 0x10000;
	res = eie != 0;

	if (!eie)
		return 0;

	asm (".p2align 3");
	do {
		asm volatile ("di");
		asm volatile ("sync.p");
		asm volatile ("mfc0\t%0, $12" : "=r" (eie));
		eie &= 0x10000;
	} while (eie);

	return res;
}

int EIntr()
{
	int eie;

	asm volatile ("mfc0\t%0, $12" : "=r" (eie));
	eie &= 0x10000;
	asm volatile ("ei");

	return eie != 0;
}

struct rpc_data {
	int	pid;
	void	*pkt_table;
	int	pkt_table_len;
	int	unused1;
	int	unused2;
	u8	*rdata_table;
	int	rdata_table_len;
	u8	*client_table;
	int	client_table_len;
	int	rdata_table_idx;
	void	*active_queue;
};

extern struct rpc_data _sif_rpc_data;

SifRpcDataQueue_t *
SifSetRpcQueue(SifRpcDataQueue_t *qd, int thread_id)
{
	SifRpcDataQueue_t *queue = NULL;

	DI();

	qd->thread_id = thread_id;
	qd->active = 0;
	qd->link   = NULL;
	qd->start  = NULL;
	qd->end    = NULL;
	qd->next   = NULL;

	if (_sif_rpc_data.active_queue == NULL) {
		_sif_rpc_data.active_queue = qd;
	} else {
		queue = _sif_rpc_data.active_queue;

		while (queue->next!=NULL) queue = queue->next;

		queue->next = qd;
	}

	EI();

	return queue;
}

SifRpcDataQueue_t *
SifRemoveRpcQueue(SifRpcDataQueue_t *qd)
{
	SifRpcDataQueue_t *queue;

	DI();

	if((queue = _sif_rpc_data.active_queue) == qd)
	{
		_sif_rpc_data.active_queue = queue->next;
	} else {
		while(queue != NULL)
		{
			if(queue->next == qd)
			{
				queue->next = qd->next;
				break;
			}

			queue = queue->next;
		}
	}

	EI();

	return queue;
}

SifRpcServerData_t *
SifRegisterRpc(SifRpcServerData_t *sd,
		int sid, SifRpcFunc_t func, void *buff, SifRpcFunc_t cfunc,
		void *cbuff, SifRpcDataQueue_t *qd)
{
	SifRpcServerData_t *server;

	DI();

	sd->link  = NULL;
	sd->next  = NULL;
	sd->sid   = sid;
	sd->func  = func;
	sd->buff  = buff;
	sd->cfunc = cfunc;
	sd->cbuff = cbuff;
	sd->base  = qd;

	if (!(server = qd->link)) {
		qd->link = sd;
	} else {
		while ((server = server->next))
			;

		server->next = sd;
	}

	EI();

	return server;
}

SifRpcServerData_t *
SifRemoveRpc(SifRpcServerData_t *sd, SifRpcDataQueue_t *queue)
{
	SifRpcServerData_t *server;

	DI();

	if((server = queue->link) == sd)
	{
		queue->link = server->link;
	} else {
		while(server != NULL)
		{
			if(server->link == sd)
			{
				server->link = sd->link;
				break;
			}

			server = server->link;
		}
	}

	EI();

	return server;
}

int memcmp(const void *s1, const void *s2, unsigned int length)
{
	const char *a = s1;
	const char *b = s2;

	while (length--) {
		if (*a++ != *b++)
			return 1;
	}

	return 0;
}

int strncmp( const char * s1, const char * s2, size_t n )
{
    while ( n && *s1 && ( *s1 == *s2 ) )
    {
        ++s1;
        ++s2;
        --n;
    }
    if ( n == 0 )
    {
        return 0;
    }
    else
    {
        return ( *(unsigned char *)s1 - *(unsigned char *)s2 );
    }
}

unsigned long long __udivdi3(unsigned long long a, unsigned long long b) {
	if(a > 0xFFFFFFFF) {
		return (unsigned long long)(((unsigned int)a >> 16ull) / (unsigned int)b) << 16ull;
	}
	return ((unsigned int)a / (unsigned int)b);
}

long long __divdi3(long long a, long long b) {
	return ((int)a / (int)b);
}
