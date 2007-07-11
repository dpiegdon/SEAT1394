#!/bin/sh

function resolve_symbol() {
	HEX_ADR="0x`grep -A1 "\<$1\>" dmashellcode.lst | grep -v EQU | egrep '\<(db|dw|dd)\>' | gawk '{print $2}'`"
	echo $HEX_ADR
}

CHILD_IS_DEAD=`resolve_symbol child_is_dead`
CHILD_IS_DEAD_ACK=`resolve_symbol child_is_dead_ACK`
TERMINATE_CHILD=`resolve_symbol terminate_child`

RFRM_WRITER_POS=`resolve_symbol rfrm_writer_pos`
RFRM_READER_POS=`resolve_symbol rfrm_reader_pos`

RTO_WRITER_POS=`resolve_symbol rto_writer_pos`
RTO_READER_POS=`resolve_symbol rto_reader_pos`

cat > dmashellcode.h << H_END

#ifndef __DMASHELLCODE_H__
# define __DMASHELLCODE_H__

# define CHILD_IS_DEAD		$CHILD_IS_DEAD
# define CHILD_IS_DEAD_ACK	$CHILD_IS_DEAD_ACK
# define TERMINATE_CHILD	$TERMINATE_CHILD

# define RFRM_WRITER_POS	$RFRM_WRITER_POS
# define RFRM_READER_POS	$RFRM_READER_POS
# define RFRM_BUFFER		0x0

# define RTO_WRITER_POS		$RTO_WRITER_POS
# define RTO_READER_POS		$RTO_READER_POS
# define RTO_BUFFER		0x0

#endif // __DMASHELLCODE_H__

H_END

