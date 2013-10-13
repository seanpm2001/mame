/***************************************************************************

    netlib.c

    Discrete netlist implementation.

****************************************************************************

    Couriersud reserves the right to license the code under a less restrictive
    license going forward.

    Copyright Nicola Salmoria and the MAME team
    All rights reserved.

    Redistribution and use of this code or any derivative works are permitted
    provided that the following conditions are met:

    * Redistributions may not be sold, nor may they be used in a commercial
    product or activity.

    * Redistributions that are modified from the original source must include the
    complete source code, including the source code for all components used by a
    binary built from the modified sources. However, as a special exception, the
    source code distributed need not include anything that is normally distributed
    (in either source or binary form) with the major components (compiler, kernel,
    and so on) of the operating system on which the executable runs, unless that
    component itself accompanies the executable.

    * Redistributions must reproduce the above copyright notice, this list of
    conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.


****************************************************************************/

#include "net_lib.h"

NETLIB_CONSTRUCTOR(netdev_logic_input)
{
	register_output("Q", m_Q);
}

NETLIB_UPDATE(netdev_logic_input)
{
}

NETLIB_CONSTRUCTOR(netdev_analog_input)
{
	register_output("Q", m_Q);
}

NETLIB_UPDATE(netdev_analog_input)
{
}

NETLIB_CONSTRUCTOR(netdev_log)
{
	register_input("I", m_I);
}

NETLIB_UPDATE(netdev_log)
{
	printf("%s: %d %d\n", name(), (UINT32) (netlist().time().as_raw() / 1000000), INPVAL(m_I));
}

NETLIB_CONSTRUCTOR(netdev_clock)
{
	register_output("Q", m_Q);
	//register_input("FB", m_feedback);

	register_param("FREQ", m_freq, 7159000.0 * 5);
	m_inc = netlist_time::from_hz(m_freq.Value()*2);

	register_link_internal(m_feedback, m_Q);

}

NETLIB_UPDATE_PARAM(netdev_clock)
{
	m_inc = netlist_time::from_hz(m_freq.Value()*2);
}

NETLIB_UPDATE(netdev_clock)
{
	m_Q.setToNoCheck(!m_Q.new_Q(), m_inc  );
}

NETLIB_UPDATE_PARAM(netdev_mainclock)
{
	m_inc = netlist_time::from_hz(m_freq.Value()*2);
}

NETLIB_UPDATE(netdev_mainclock)
{
	*m_Q.new_Q_ptr() = !m_Q.new_Q();
	m_Q.set_time(m_netlist.time() + m_inc);
}

NETLIB_CONSTRUCTOR(nicMultiSwitch)
{
	static const char *sIN[8] = { "i1", "i2", "i3", "i4", "i5", "i6", "i7", "i8" };
	int i;

	m_position = 0;
	m_low.initial(0);

	for (i=0; i<8; i++)
	{
		register_input(sIN[i], m_I[i]);
		//register_link_internal(m_I[i], m_low);
		m_I[i].set_output( GETINPPTR(m_low));
	}
	register_param("POS", m_POS);
	register_output("Q", m_Q);

	m_variable_input_count = true;
}

NETLIB_UPDATE(nicMultiSwitch)
{
	assert(m_position<8);
	m_Q.setTo(INPANALOG(m_I[m_position]), NLTIME_FROM_NS(1));
}

NETLIB_UPDATE_PARAM(nicMultiSwitch)
{
	m_position = m_POS.ValueInt();
	update();
}

NETLIB_CONSTRUCTOR(nicMixer8)
{
	static const char *sI[8] = { "I1", "I2", "I3", "I4", "I5", "I6", "I7", "I8" };
	static const char *sR[8] = { "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8" };
	int i;

	m_low.initial(0);

	for (i=0; i<8; i++)
	{
		register_input(sI[i], m_I[i]);
		m_I[i].set_output(GETINPPTR(m_low));
		register_param(sR[i], m_R[i], 1e12);
	}
	register_output("Q", m_Q);

	m_variable_input_count = true;
}

NETLIB_UPDATE(nicMixer8)
{
	int i;
	double r = 0;

	for (i=0; i<8; i++)
	{
		r += m_w[i] * INPANALOG(m_I[i]);
	}
	m_Q.setTo(r, NLTIME_IMMEDIATE);
}

NETLIB_UPDATE_PARAM(nicMixer8)
{
	double t = 0;
	int i;

	for (i=0; i<8; i++)
		t += 1.0 / m_R[i].Value();
	t = 1.0 / t;

	for (i=0; i<8; i++)
		m_w[i] = t / m_R[i].Value();
}



NETLIB_CONSTRUCTOR(nicRSFF)
{
	register_input("S", m_S);
	register_input("R", m_R);
	register_output("Q", m_Q);
	register_output("QQ", m_QQ);
	m_Q.initial(0);
	m_QQ.initial(1);
}

NETLIB_UPDATE(nicRSFF)
{
	if (INPVAL(m_S))
	{
		m_Q.set();
		m_QQ.clear();
	}
	else if (INPVAL(m_R))
	{
		m_Q.clear();
		m_QQ.set();
	}
}


NETLIB_CONSTRUCTOR(nicNE555N_MSTABLE)
{
	register_input("TRIG", m_trigger);
	register_input("CV", m_CV);

	register_output("Q", m_Q);
	register_param("R", m_R);
	register_param("C", m_C);
	register_param("VS", m_VS, 5.0);
	register_param("VL", m_VL, 0.0 *5.0);

	m_THRESHOLD_OUT.set_netdev(this);
	register_link_internal(m_THRESHOLD, m_THRESHOLD_OUT);

	m_Q.initial(5.0 * 0.4);
	m_last = false;
}

INLINE double nicNE555N_cv(nicNE555N_MSTABLE &dev)
{
	return (dev.m_CV.is_highz() ? 0.67 * dev.m_VS.Value() : dev.INPANALOG(dev.m_CV));
}

INLINE double nicNE555N_clamp(nicNE555N_MSTABLE &dev, const double v, const double a, const double b)
{
	double ret = v;
	if (ret >  dev.m_VS.Value() - a)
		ret = dev.m_VS.Value() - a;
	if (ret < b)
		ret = b;
	return ret;
}

NETLIB_UPDATE_PARAM(nicNE555N_MSTABLE)
{
}

NETLIB_UPDATE(nicNE555N_MSTABLE)
{
	update_param(); // FIXME : m_CV should be on a sub device ...

	double vt = nicNE555N_clamp(*this, nicNE555N_cv(*this), 0.7, 1.4);
	bool bthresh = (INPANALOG(m_THRESHOLD) > vt);
	bool btrig = (INPANALOG(m_trigger) > nicNE555N_clamp(*this, nicNE555N_cv(*this) * 0.5, 0.7, 1.4));
	bool out = m_last;

	if (!btrig)
	{
		out = true;
	}
	else if (bthresh)
	{
		out = false;
	}

	if (!m_last && out)
	{
		double vl = m_VL.Value();
		double time;

		// FIXME : m_CV should be on a sub device ...

		// TI datasheet states minimum pulse of 10 us
		if (vt<vl)
			time = 10;
		else
		{
			time = - log((m_VS.Value()-vt)/(m_VS.Value()-vl)) * m_R.Value() * m_C.Value() * 1.0e6; // in us
			if (time < 10.0)
				time = 10.0;
		}

		m_Q.setToNoCheckPS(m_VS.Value() * 0.7, NLTIME_FROM_NS(100));
		m_THRESHOLD_OUT.setTo(m_VS.Value(), NLTIME_FROM_US(time ));
	}
	else if (m_last && !out)
	{
		m_Q.setToNoCheckPS(0.25, NLTIME_FROM_NS(100));
		m_THRESHOLD_OUT.setTo(0.0, NLTIME_FROM_NS(1));
	}
	m_last = out;
}

NETLIB_CONSTRUCTOR(nic7404)
{
	register_input("I1", m_I);
	register_output("Q", m_Q);
	m_Q.initial(1);
}

NETLIB_UPDATE(nic7404)
{
	static const netlist_time delay[2] = { NLTIME_FROM_NS(15), NLTIME_FROM_NS(22) };
	UINT8 t = (INPVAL(m_I)) ^ 1;
	m_Q.setTo(t, delay[t]);
}

NETLIB_CONSTRUCTOR(nic7486)
{
	register_input("I1", m_I0);
	register_input("I2", m_I1);
	register_output("Q", m_Q);
}

NETLIB_UPDATE(nic7486)
{
	static const netlist_time delay[2] = { NLTIME_FROM_NS(15), NLTIME_FROM_NS(22) };
	UINT8 t = INPVAL(m_I0) ^ INPVAL(m_I1);
	m_Q.setTo(t, delay[t]);
}

NETLIB_CONSTRUCTOR(nic7448)
, sub(setup, "sub")
{
	sub.m_state = 0;

	register_input(sub, "A0", sub.m_A0);
	register_input(sub, "A1", sub.m_A1);
	register_input(sub, "A2", sub.m_A2);
	register_input(sub, "A3", sub.m_A3);
	register_input("LTQ", m_LTQ);
	register_input("BIQ", m_BIQ);
	register_input(sub, "RBIQ",sub.m_RBIQ);

	register_output(sub, "a", sub.m_a);
	register_output(sub, "b", sub.m_b);
	register_output(sub, "c", sub.m_c);
	register_output(sub, "d", sub.m_d);
	register_output(sub, "e", sub.m_e);
	register_output(sub, "f", sub.m_f);
	register_output(sub, "g", sub.m_g);
}

NETLIB_UPDATE(nic7448)
{

	if (INPVAL(m_BIQ) && !INPVAL(m_LTQ))
	{
		sub.update_outputs(8);
	}
	else if (!INPVAL(m_BIQ))
	{
		sub.update_outputs(15);
	}

	if (!INPVAL(m_BIQ) || (INPVAL(m_BIQ) && !INPVAL(m_LTQ)))
	{
		sub.m_A0.inactivate();
		sub.m_A1.inactivate();
		sub.m_A2.inactivate();
		sub.m_A3.inactivate();
		sub.m_RBIQ.inactivate();
	} else {
		sub.m_RBIQ.activate();
		sub.m_A3.activate();
		sub.m_A2.activate();
		sub.m_A1.activate();
		sub.m_A0.activate();
		sub.update();
	}

}

NETLIB_UPDATE(nic7448_sub)
{
	UINT8 v;

	v = (INPVAL(m_A0) << 0) | (INPVAL(m_A1) << 1) | (INPVAL(m_A2) << 2) | (INPVAL(m_A3) << 3);
	if ((!INPVAL(m_RBIQ) && (v==0)))
			v = 15;
	update_outputs(v);
}

NETLIB_FUNC_VOID(nic7448_sub, update_outputs, (UINT8 v))
{
	assert(v<16);
	if (v != m_state)
	{
		m_a.setTo(tab7448[v][0], NLTIME_FROM_NS(100));
		m_b.setTo(tab7448[v][1], NLTIME_FROM_NS(100));
		m_c.setTo(tab7448[v][2], NLTIME_FROM_NS(100));
		m_d.setTo(tab7448[v][3], NLTIME_FROM_NS(100));
		m_e.setTo(tab7448[v][4], NLTIME_FROM_NS(100));
		m_f.setTo(tab7448[v][5], NLTIME_FROM_NS(100));
		m_g.setTo(tab7448[v][6], NLTIME_FROM_NS(100));
		m_state = v;
	}
}

const UINT8 nic7448_sub::tab7448[16][7] =
{
		{   1, 1, 1, 1, 1, 1, 0 },  /* 00 - not blanked ! */
		{   0, 1, 1, 0, 0, 0, 0 },  /* 01 */
		{   1, 1, 0, 1, 1, 0, 1 },  /* 02 */
		{   1, 1, 1, 1, 0, 0, 1 },  /* 03 */
		{   0, 1, 1, 0, 0, 1, 1 },  /* 04 */
		{   1, 0, 1, 1, 0, 1, 1 },  /* 05 */
		{   0, 0, 1, 1, 1, 1, 1 },  /* 06 */
		{   1, 1, 1, 0, 0, 0, 0 },  /* 07 */
		{   1, 1, 1, 1, 1, 1, 1 },  /* 08 */
		{   1, 1, 1, 0, 0, 1, 1 },  /* 09 */
		{   0, 0, 0, 1, 1, 0, 1 },  /* 10 */
		{   0, 0, 1, 1, 0, 0, 1 },  /* 11 */
		{   0, 1, 0, 0, 0, 1, 1 },  /* 12 */
		{   1, 0, 0, 1, 0, 1, 1 },  /* 13 */
		{   0, 0, 0, 1, 1, 1, 1 },  /* 14 */
		{   0, 0, 0, 0, 0, 0, 0 },  /* 15 */
};

NETLIB_CONSTRUCTOR(nic7450)
{
	register_input("I1", m_I0);
	register_input("I2", m_I1);
	register_input("I3", m_I2);
	register_input("I4", m_I3);
	register_output("Q", m_Q);
}

NETLIB_UPDATE(nic7450)
{
	UINT8 t1 = INPVAL(m_I0) & INPVAL(m_I1);
	UINT8 t2 = INPVAL(m_I2) & INPVAL(m_I3);
	UINT8 t =  (t1 | t2) ^ 1;
	m_Q.setTo(t, t ? NLTIME_FROM_NS(22) : NLTIME_FROM_NS(15));
}

INLINE void nic7474_newstate(const UINT8 state, ttl_output_t &Q, ttl_output_t &QQ)
{
	const netlist_time delay[2] = { NLTIME_FROM_NS(25), NLTIME_FROM_NS(40) };
	//printf("%s %d %d %d\n", "7474", state, Q.Q(), QQ.Q());
	Q.setTo(state, delay[state]);
	QQ.setTo(!state, delay[!state]);
}

NETLIB_UPDATE(nic7474sub)
{
	//if (!INP_LAST(m_clk) & INP(m_clk))
	{
		nic7474_newstate(m_nextD, m_Q, m_QQ);
		m_clk.inactivate();
	}
}

NETLIB_UPDATE(nic7474)
{
	sub.m_nextD = INPVAL(m_D);
	if (!INPVAL(m_preQ))
	{
		nic7474_newstate(1, sub.m_Q, sub.m_QQ);
		sub.m_clk.inactivate();
	}
	else if (!INPVAL(m_clrQ))
	{
		nic7474_newstate(0, sub.m_Q, sub.m_QQ);
		sub.m_clk.inactivate();
	}
	else
		sub.m_clk.activate_lh();
}

NETLIB_CONSTRUCTOR(nic7474)
, sub(setup, "sub")
{

	register_input(sub, "CLK",  sub.m_clk, net_input_t::INP_STATE_LH);
	register_input("D",    m_D);
	register_input("CLRQ", m_clrQ);
	register_input("PREQ", m_preQ);

	register_output(sub, "Q",   sub.m_Q);
	register_output(sub, "QQ",  sub.m_QQ);

	sub.m_Q.initial(1);
	sub.m_QQ.initial(0);
}

NETLIB_CONSTRUCTOR(nic7483)
{
	m_lastr = 0;

	register_input("A1", m_A1);
	register_input("A2", m_A2);
	register_input("A3", m_A3);
	register_input("A4", m_A4);
	register_input("B1", m_B1);
	register_input("B2", m_B2);
	register_input("B3", m_B3);
	register_input("B4", m_B4);
	register_input("CI", m_CI);

	register_output("SA", m_SA);
	register_output("SB", m_SB);
	register_output("SC", m_SC);
	register_output("SD", m_SD);
	register_output("CO", m_CO);
}

NETLIB_UPDATE(nic7483)
{
	UINT8 a = (INPVAL(m_A1) << 0) | (INPVAL(m_A2) << 1) | (INPVAL(m_A3) << 2) | (INPVAL(m_A4) << 3);
	UINT8 b = (INPVAL(m_B1) << 0) | (INPVAL(m_B2) << 1) | (INPVAL(m_B3) << 2) | (INPVAL(m_B4) << 3);

	UINT8 r = a + b + INPVAL(m_CI);

	if (r != m_lastr)
	{
		m_lastr = r;
		m_SA.setTo((r >> 0) & 1, NLTIME_FROM_NS(23));
		m_SB.setTo((r >> 1) & 1, NLTIME_FROM_NS(23));
		m_SC.setTo((r >> 2) & 1, NLTIME_FROM_NS(23));
		m_SD.setTo((r >> 3) & 1, NLTIME_FROM_NS(23));
		m_CO.setTo((r >> 4) & 1, NLTIME_FROM_NS(23));
	}
}

NETLIB_CONSTRUCTOR(nic7490)
{
	m_cnt = 0;

	register_input("CLK", m_clk);
	register_input("R1",  m_R1);
	register_input("R2",  m_R2);
	register_input("R91", m_R91);
	register_input("R92", m_R92);

	register_output("QA", m_Q[0]);
	register_output("QB", m_Q[1]);
	register_output("QC", m_Q[2]);
	register_output("QD", m_Q[3]);
}

NETLIB_UPDATE(nic7490)
{
	if (INPVAL(m_R91) & INPVAL(m_R92))
	{
		m_cnt = 9;
		update_outputs();
	}
	else if (INPVAL(m_R1) & INPVAL(m_R2))
	{
		m_cnt = 0;
		update_outputs();
	}
	else if (INPVAL_LAST(m_clk) & !INPVAL(m_clk))
	{
		m_cnt++;
		if (m_cnt >= 10)
			m_cnt = 0;
		update_outputs();
	}
}
#if 0
NETLIB_FUNC_VOID(nic7490, update_outputs)
{
	m_QA.setTo((m_cnt >> 0) & 1, NLTIME_FROM_NS(18));
	m_QB.setTo((m_cnt >> 1) & 1, NLTIME_FROM_NS(36));
	m_QC.setTo((m_cnt >> 2) & 1, NLTIME_FROM_NS(54));
	m_QD.setTo((m_cnt >> 3) & 1, NLTIME_FROM_NS(72));
}
#else
NETLIB_FUNC_VOID(nic7490, update_outputs, (void))
{
	const netlist_time delay[4] = { NLTIME_FROM_NS(18), NLTIME_FROM_NS(36), NLTIME_FROM_NS(54), NLTIME_FROM_NS(72) };
	for (int i=0; i<4; i++)
		m_Q[i].setTo((m_cnt >> i) & 1, delay[i]);

		//m_QA.setToPS((m_cnt >> 0) & 1, delay[0]);
		//m_QB.setToPS((m_cnt >> 1) & 1, delay[1]);
		//m_QC.setToPS((m_cnt >> 2) & 1, delay[2]);
		//m_QD.setToPS((m_cnt >> 3) & 1, delay[3]);
}
#endif
#if !USE_OLD7493
NETLIB_CONSTRUCTOR(nic7493)
, A(setup, "A")
, B(setup, "B")
, C(setup, "C")
, D(setup, "D")
{
	register_input(A, "CLKA", A.m_I, net_input_t::INP_STATE_HL);
	register_input(B, "CLKB", B.m_I, net_input_t::INP_STATE_HL);
	register_input("R1",  m_R1);
	register_input("R2",  m_R2);

	register_output(A, "QA", A.m_Q);
	register_output(B, "QB", B.m_Q);
	register_output(C, "QC", C.m_Q);
	register_output(D, "QD", D.m_Q);

	//B.register_link_internal(B.m_I, A.m_Q);
	register_link_internal(C, C.m_I, B.m_Q);
	register_link_internal(D, D.m_I, C.m_Q);

}

NETLIB_UPDATE(nic7493ff)
{
	//if INP_LAST(m_I) && !INP(m_I))
		m_Q.setToNoCheck(!m_Q.new_Q(), NLTIME_FROM_NS(18));
}

NETLIB_UPDATE(nic7493)
{
	net_sig_t r = INPVAL(m_R1) & INPVAL(m_R2);

	if (r)
	{
		//printf("%s reset\n", name());
		A.m_I.inactivate();
		B.m_I.inactivate();
		C.m_I.inactivate();
		D.m_I.inactivate();
		A.m_Q.setTo(0, NLTIME_FROM_NS(40));
		B.m_Q.setTo(0, NLTIME_FROM_NS(40));
		C.m_Q.setTo(0, NLTIME_FROM_NS(40));
		D.m_Q.setTo(0, NLTIME_FROM_NS(40));
	}
	else
	{
		A.m_I.activate_hl();
		B.m_I.activate_hl();
		C.m_I.activate_hl();
		D.m_I.activate_hl();
		//printf("%s enable\n", name());
	}
}
#else

NETLIB_CONSTRUCTOR(nic7493)
{
	m_cnt = 0;

	register_input("CLKA", m_CLK, net_input_t::INP_STATE_HL);
	register_input("CLKB", m_CLKB, net_input_t::INP_STATE_HL);
	register_input("R1",  m_R1);
	register_input("R2",  m_R2);

	register_output("QA", m_QA);
	register_output("QB", m_QB);
	register_output("QC", m_QC);
	register_output("QD", m_QD);
}

NETLIB_UPDATE(nic7493)
{
	//UINT8 old_clk = m_lastclk;
	//m_lastclk = INPVAL(m_clk);

	//printf("%s %d %d %d %d %d %d\n", name(), m_cnt, old_clk, m_QA.Q(), m_QB.Q(), m_QC.Q(), m_QD.Q());
	if (INPVAL(m_R1) & INPVAL(m_R2))
	{
		if (m_cnt > 0)
		{
			m_cnt = 0;
			m_QA.setTo(0, NLTIME_FROM_NS(40));
			m_QB.setTo(0, NLTIME_FROM_NS(40));
			m_QC.setTo(0, NLTIME_FROM_NS(40));
			m_QD.setTo(0, NLTIME_FROM_NS(40));
		}
		//m_CLK.inactivate();
	}
	//else if (old_clk & !m_lastclk)
	else {
		//m_CLK.activate_hl();
		if (INPVAL_LAST(m_CLK) & !INPVAL(m_CLK))
		{
			m_cnt++;
			m_cnt &= 0x0f;
			update_outputs();
		}
	}
}

NETLIB_FUNC_VOID(nic7493, update_outputs, (void))
{
	if (m_cnt & 1)
		m_QA.setToNoCheck(1, NLTIME_FROM_NS(16));
	else
	{
		m_QA.setToNoCheck(0, NLTIME_FROM_NS(16));
		switch (m_cnt)
		{
		case 0x00:
			m_QB.setToNoCheck(0, NLTIME_FROM_NS(34));
			m_QC.setToNoCheck(0, NLTIME_FROM_NS(48));
			m_QD.setToNoCheck(0, NLTIME_FROM_NS(70));
			break;
		case 0x02:
		case 0x06:
		case 0x0A:
		case 0x0E:
			m_QB.setToNoCheck(1, NLTIME_FROM_NS(34));
			break;
		case 0x04:
		case 0x0C:
			m_QB.setToNoCheck(0, NLTIME_FROM_NS(34));
			m_QC.setToNoCheck(1, NLTIME_FROM_NS(48));
			break;
		case 0x08:
			m_QB.setToNoCheck(0, NLTIME_FROM_NS(34));
			m_QC.setToNoCheck(0, NLTIME_FROM_NS(48));
			m_QD.setToNoCheck(1, NLTIME_FROM_NS(70));
			break;
		}
	}
}
#endif

NETLIB_CONSTRUCTOR(nic74107A)
, sub(setup, "sub")
{
	register_input(sub, "CLK", sub.m_clk, net_input_t::INP_STATE_HL);
	register_input("J", m_J);
	register_input("K", m_K);
	register_input("CLRQ", m_clrQ);
	register_output(sub, "Q", sub.m_Q);
	register_output(sub, "QQ", sub.m_QQ);

	sub.m_Q.initial(0);
	sub.m_QQ.initial(1);
}

INLINE void nic74107A_newstate(UINT8 state, ttl_output_t &Q, ttl_output_t &QQ)
{
	const netlist_time delay[2] = { NLTIME_FROM_NS(40), NLTIME_FROM_NS(25) };
#if 1
	Q.setTo(state, delay[1-state]);
	QQ.setTo(1-state, delay[state]);
#else
	if (state != Q.new_Q())
	{
		Q.setToNoCheck(state, delay[1-state]);
		QQ.setToNoCheck(1-state, delay[state]);
	}
#endif
}

NETLIB_UPDATE(nic74107Asub)
{
	{
		nic74107A_newstate((!m_Q.new_Q() & m_Q1) | (m_Q.new_Q() & m_Q2) | m_F, m_Q, m_QQ);
		if (!m_Q1)
			m_clk.inactivate();
	}
}

NETLIB_UPDATE(nic74107A)
{
	if (INPVAL(m_J) & INPVAL(m_K))
	{
		sub.m_Q1 = 1;
		sub.m_Q2 = 0;
		sub.m_F  = 0;
	}
	else if (!INPVAL(m_J) & INPVAL(m_K))
	{
		sub.m_Q1 = 0;
		sub.m_Q2 = 0;
		sub.m_F  = 0;
	}
	else if (INPVAL(m_J) & !INPVAL(m_K))
	{
		sub.m_Q1 = 0;
		sub.m_Q2 = 0;
		sub.m_F  = 1;
	}
	else
	{
		sub.m_Q1 = 0;
		sub.m_Q2 = 1;
		sub.m_F  = 0;
		sub.m_clk.inactivate();
	}
	if (!INPVAL(m_clrQ))
	{
		sub.m_clk.inactivate();
		nic74107A_newstate(0, sub.m_Q, sub.m_QQ);
	}
	if (!sub.m_Q2 && INPVAL(m_clrQ))
		sub.m_clk.activate_hl();
}

NETLIB_CONSTRUCTOR(nic74153)
{
	register_input("A1", m_I[0]);
	register_input("A2", m_I[1]);
	register_input("A3", m_I[2]);
	register_input("A4", m_I[3]);
	register_input("A", m_A);
	register_input("B", m_B);
	register_input("GA", m_GA);

	register_output("AY", m_AY);
}

NETLIB_UPDATE(nic74153)
{
	const netlist_time delay[2] = { NLTIME_FROM_NS(23), NLTIME_FROM_NS(18) };
	if (!INPVAL(m_GA))
	{
		UINT8 chan = (INPVAL(m_A) | (INPVAL(m_B)<<1));
		UINT8 t = INPVAL(m_I[chan]);
		m_AY.setTo(t, delay[t] ); /* data to y only, FIXME */
	}
	else
	{
		m_AY.setTo(0, delay[0]);
	}
}

NETLIB_CONSTRUCTOR(nic9316)
, sub(setup, "sub")
{
	sub.m_cnt = 0;
	sub.m_loadq = 1;
	sub.m_ent = 1;

	register_input(sub, "CLK", sub.m_clk, net_input_t::INP_STATE_LH);

	register_input("ENP", m_ENP);
	register_input("ENT", m_ENT);
	register_input("CLRQ", m_CLRQ);
	register_input("LOADQ", m_LOADQ);

	register_input(sub, "A", sub.m_A, net_input_t::INP_STATE_PASSIVE);
	register_input(sub, "B", sub.m_B, net_input_t::INP_STATE_PASSIVE);
	register_input(sub, "C", sub.m_C, net_input_t::INP_STATE_PASSIVE);
	register_input(sub, "D", sub.m_D, net_input_t::INP_STATE_PASSIVE);

	register_output(sub, "QA", sub.m_QA);
	register_output(sub, "QB", sub.m_QB);
	register_output(sub, "QC", sub.m_QC);
	register_output(sub, "QD", sub.m_QD);
	register_output(sub, "RC", sub.m_RC);

}

NETLIB_UPDATE(nic9316_sub)
{
	if (m_loadq)
	{
		m_cnt = ( m_cnt + 1) & 0x0f;
		update_outputs();
	}
	else
	{
		m_cnt = (INPVAL_PASSIVE(m_D) << 3) | (INPVAL_PASSIVE(m_C) << 2) | (INPVAL_PASSIVE(m_B) << 1) | (INPVAL_PASSIVE(m_A) << 0);
		update_outputs_all();
	}
	m_RC.setTo(m_ent & (m_cnt == 0x0f), NLTIME_FROM_NS(20));
}

NETLIB_UPDATE(nic9316)
{
	sub.m_loadq = INPVAL(m_LOADQ);
	sub.m_ent = INPVAL(m_ENT);

	if ((!sub.m_loadq || (sub.m_ent & INPVAL(m_ENP))) & INPVAL(m_CLRQ))
	{
		sub.m_clk.activate_lh();
	}
	else
	{
		sub.m_clk.inactivate();
		if (!INPVAL(m_CLRQ) & (sub.m_cnt>0))
		{
			sub.m_cnt = 0;
			sub.update_outputs();
			sub.m_RC.setTo(0, NLTIME_FROM_NS(20));
			return;
		}
	}
	sub.m_RC.setTo(sub.m_ent & (sub.m_cnt == 0x0f), NLTIME_FROM_NS(20));
}

NETLIB_FUNC_VOID(nic9316_sub, update_outputs_all, (void))
{
	const netlist_time out_delay = NLTIME_FROM_NS(20);
	m_QA.setTo((m_cnt >> 0) & 1, out_delay);
	m_QB.setTo((m_cnt >> 1) & 1, out_delay);
	m_QC.setTo((m_cnt >> 2) & 1, out_delay);
	m_QD.setTo((m_cnt >> 3) & 1, out_delay);
}

NETLIB_FUNC_VOID(nic9316_sub, update_outputs, (void))
{
	const netlist_time out_delay = NLTIME_FROM_NS(20);
#if 1
	m_QA.setTo((m_cnt >> 0) & 1, out_delay);
	m_QB.setTo((m_cnt >> 1) & 1, out_delay);
	m_QC.setTo((m_cnt >> 2) & 1, out_delay);
	m_QD.setTo((m_cnt >> 3) & 1, out_delay);
#else
	if ((m_cnt & 1) == 1)
		m_QA.setToNoCheck(1, out_delay);
	else
	{
		m_QA.setToNoCheck(0, out_delay);
		switch (m_cnt)
		{
		case 0x00:
			m_QB.setToNoCheck(0, out_delay);
			m_QC.setToNoCheck(0, out_delay);
			m_QD.setToNoCheck(0, out_delay);
			break;
		case 0x02:
		case 0x06:
		case 0x0A:
		case 0x0E:
			m_QB.setToNoCheck(1, out_delay);
			break;
		case 0x04:
		case 0x0C:
			m_QB.setToNoCheck(0, out_delay);
			m_QC.setToNoCheck(1, out_delay);
			break;
		case 0x08:
			m_QB.setToNoCheck(0, out_delay);
			m_QC.setToNoCheck(0, out_delay);
			m_QD.setToNoCheck(1, out_delay);
			break;
		}

	}
#endif
}

#define ENTRY(_nic, _name) new net_device_t_factory< _nic >( # _name, # _nic ),

static const net_device_t_base_factory *netregistry[] =
{
	ENTRY(netdev_ttl_const,     NETDEV_TTL_CONST)
	ENTRY(netdev_analog_const,  NETDEV_ANALOG_CONST)
	ENTRY(netdev_logic_input,   NETDEV_LOGIC_INPUT)
	ENTRY(netdev_analog_input,  NETDEV_ANALOG_INPUT)
	ENTRY(netdev_log,		   	NETDEV_LOG)
	ENTRY(netdev_clock,         NETDEV_CLOCK)
	ENTRY(netdev_mainclock,     NETDEV_MAINCLOCK)
	ENTRY(netdev_callback,      NETDEV_CALLBACK)
	ENTRY(nicMultiSwitch,       NETDEV_SWITCH2)
	ENTRY(nicRSFF,              NETDEV_RSFF)
	ENTRY(nicMixer8,            NETDEV_MIXER)
	ENTRY(nic7400,              TTL_7400_NAND)
	ENTRY(nic7402,              TTL_7402_NOR)
	ENTRY(nic7404,              TTL_7404_INVERT)
	ENTRY(nic7410,              TTL_7410_NAND)
	ENTRY(nic7420,              TTL_7420_NAND)
	ENTRY(nic7425,              TTL_7425_NOR)
	ENTRY(nic7427,              TTL_7427_NOR)
	ENTRY(nic7430,              TTL_7430_NAND)
	ENTRY(nic7450,              TTL_7450_ANDORINVERT)
	ENTRY(nic7486,              TTL_7486_XOR)
	ENTRY(nic7448,              TTL_7448)
	ENTRY(nic7474,              TTL_7474)
	ENTRY(nic7483,              TTL_7483)
	ENTRY(nic7490,              TTL_7490)
	ENTRY(nic7493,              TTL_7493)
	ENTRY(nic74107,             TTL_74107)
	ENTRY(nic74107A,            TTL_74107A)
	ENTRY(nic74153,             TTL_74153)
	ENTRY(nic9316,              TTL_9316)
	ENTRY(nicNE555N_MSTABLE,    NE555N_MSTABLE)
	NULL
};

net_device_t *net_create_device_by_classname(const char *classname, netlist_setup_t &setup, const char *icname)
{
	const net_device_t_base_factory **p = &netregistry[0];
	while (p != NULL)
	{
		if (strcmp((*p)->classname(), classname) == 0)
			return (*p)->Create(setup, icname);
		p++;
	}
	fatalerror("Class %s required for IC %s not found!\n", classname, icname);
	return NULL; // appease code analysis
}

net_device_t *net_create_device_by_name(const char *name, netlist_setup_t &setup, const char *icname)
{
	const net_device_t_base_factory **p = &netregistry[0];
	while (p != NULL)
	{
		if (strcmp((*p)->name(), name) == 0)
			return (*p)->Create(setup, icname);
		p++;
	}
	fatalerror("Class %s required for IC %s not found!\n", name, icname);
	return NULL; // appease code analysis
}
