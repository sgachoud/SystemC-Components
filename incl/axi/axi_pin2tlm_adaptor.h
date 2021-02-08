#pragma once

#include <axi/axi_tlm.h>
#include <scv4tlm/tlm_rec_initiator_socket.h>
#include <scc/initiator_mixin.h>
#include <tlm/tlm_mm.h>
#include <scc/report.h>
#include <tlm/tlm_id.h>

#include <systemc>
#include <tlm>

#include <unordered_map>
#include <memory>

namespace axi_bfm {

template <unsigned int BUSWIDTH = 32, unsigned int ADDRWIDTH = 32, unsigned int IDWIDTH = 32>
class axi_pin2tlm_adaptor : public sc_core::sc_module {
public:
    using payload_type = axi::axi_protocol_types::tlm_payload_type;
    using phase_type = axi::axi_protocol_types::tlm_phase_type;

    SC_HAS_PROCESS(axi_pin2tlm_adaptor);

    axi_pin2tlm_adaptor(sc_core::sc_module_name nm);

    scc::initiator_mixin<scv4tlm::tlm_rec_initiator_socket<BUSWIDTH,axi::axi_protocol_types>,axi::axi_protocol_types> output_socket{"output_socket"};

    sc_core::sc_in<bool> clk_i{"clk_i"};
    sc_core::sc_in<bool> resetn_i{"resetn_i"}; // active low reset

    // Write address channel signals
    sc_core::sc_in<sc_dt::sc_uint<IDWIDTH>> aw_id_i{"aw_id_i"};
    sc_core::sc_in<sc_dt::sc_uint<ADDRWIDTH>> aw_addr_i{"aw_addr_i"};
    sc_core::sc_out<bool> aw_ready_o{"aw_ready_o"};
    sc_core::sc_in<bool> aw_lock_i{"aw_lock_i"};
    sc_core::sc_in<bool> aw_valid_i{"aw_valid_i"};
    sc_core::sc_in<sc_dt::sc_uint<3>> aw_prot_i{"aw_prot_i"};
    sc_core::sc_in<sc_dt::sc_uint<3>> aw_size_i{"aw_size_i"};
    sc_core::sc_in<sc_dt::sc_uint<4>> aw_cache_i{"aw_cache_i"};
    sc_core::sc_in<sc_dt::sc_uint<2>> aw_burst_i{"aw_burst_i"};
    sc_core::sc_in<sc_dt::sc_uint<4>> aw_qos_i{"Aaw_qos_i"};
    sc_core::sc_in<sc_dt::sc_uint<4>> aw_region_i{"aw_region_i"};
    sc_core::sc_in<sc_dt::sc_uint<8>> aw_len_i{"aw_len_i"};

    // write data channel signals
    sc_core::sc_in<sc_dt::sc_biguint<BUSWIDTH>> w_data_i{"w_data_i"};
    sc_core::sc_in<sc_dt::sc_uint<BUSWIDTH/8>> w_strb_i{"w_strb_i"};
    sc_core::sc_in<bool> w_last_i{"w_last_i"};
    sc_core::sc_in<bool> w_valid_i{"w_valid_i"};
    sc_core::sc_out<bool> w_ready_o{"w_ready_o"};

    // write response channel signals
    sc_core::sc_out<bool> b_valid_o{"b_valid_o"};
    sc_core::sc_in<bool> b_ready_i{"b_ready_i"};
    sc_core::sc_out<sc_dt::sc_uint<IDWIDTH>> b_id_o{"b_id_o"};
    sc_core::sc_out<sc_dt::sc_uint<2>> b_resp_o{"b_resp_o"};

    // read address channel signals
    sc_core::sc_in<sc_dt::sc_uint<IDWIDTH>> ar_id_i{"ar_id_i"};
    sc_core::sc_in<sc_dt::sc_uint<ADDRWIDTH>> ar_addr_i{"ar_addr_i"};
    sc_core::sc_in<sc_dt::sc_uint<8>> ar_len_i{"ar_len_i"};
    sc_core::sc_in<sc_dt::sc_uint<3>> ar_size_i{"ar_size_i"};
    sc_core::sc_in<sc_dt::sc_uint<2>> ar_burst_i{"ar_burst_i"};
    sc_core::sc_in<bool> ar_lock_i{"ar_lock_i"};
    sc_core::sc_in<sc_dt::sc_uint<4>> ar_cache_i{"ar_cache_i"};
    sc_core::sc_in<sc_dt::sc_uint<3>> ar_prot_i{"ar_prot_i"};
    sc_core::sc_in<sc_dt::sc_uint<4>> ar_qos_i{"ar_qos_i"};
    sc_core::sc_in<sc_dt::sc_uint<4>> ar_region_i{"ar_region_i"};
    sc_core::sc_in<bool> ar_valid_i{"ar_valid_i"};
    sc_core::sc_out<bool> ar_ready_o{"ar_ready_o"};

    // Read data channel signals
    sc_core::sc_out<sc_dt::sc_uint<IDWIDTH>> r_id_o{"r_id_o"};
    sc_core::sc_out<sc_dt::sc_biguint<BUSWIDTH>> r_data_o{"r_data_o"};
    sc_core::sc_out<sc_dt::sc_uint<2>> r_resp_o{"r_resp_o"};
    sc_core::sc_out<bool> r_last_o{"r_last_o"};
    sc_core::sc_out<bool> r_valid_o{"r_valid_o"};
    sc_core::sc_in<bool> r_ready_i{"r_ready_i"};

    tlm::tlm_sync_enum nb_transport_bw(payload_type& trans, phase_type& phase, sc_core::sc_time& t);

private:
    void reset();
    void bus_thread();

    /**
     * a handle class holding the pointer to the current transaction and associated phase
     */
    struct trans_handle {
        //! pointer to the associated AXITLM payload
    	payload_type* payload = nullptr;
    	//! current protocol phase
    	phase_type    phase = tlm::UNINITIALIZED_PHASE;
    };

    std::unordered_map<uint8_t, std::shared_ptr<trans_handle>> active_transactions;
	auto get_active_trans(unsigned &id);
	void register_trans(unsigned int axi_id, payload_type &trans, phase_type phase);
};


/////////////////////////////////////////////////////////////////////////////////////////
// Class definition
/////////////////////////////////////////////////////////////////////////////////////////
template <unsigned int BUSWIDTH, unsigned int ADDRWIDTH, unsigned int IDWIDTH>
inline axi_pin2tlm_adaptor<BUSWIDTH, ADDRWIDTH, IDWIDTH>::axi_pin2tlm_adaptor::axi_pin2tlm_adaptor(sc_core::sc_module_name nm)
: sc_module(nm) {
    output_socket.register_nb_transport_bw([this](payload_type& trans, phase_type& phase, sc_core::sc_time& t)
    		-> tlm::tlm_sync_enum { return nb_transport_bw(trans, phase, t); });

    SC_METHOD(reset);
    sensitive << resetn_i.neg();

    SC_THREAD(bus_thread)
    sensitive << clk_i.pos();
}


template <unsigned int BUSWIDTH, unsigned int ADDRWIDTH, unsigned int IDWIDTH>
inline void axi_pin2tlm_adaptor<BUSWIDTH, ADDRWIDTH, IDWIDTH>::axi_pin2tlm_adaptor::bus_thread() {
    sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
    sc_dt::sc_biguint<BUSWIDTH> write_data{0};

    while(true) {
        wait();

        if (ar_valid_i.read())
    		ar_ready_o.write(false);
		if(r_ready_i.read()) {
			r_valid_o.write(false);
			r_last_o.write(false);
		}
		if(b_ready_i.read())
			b_valid_o.write(false);
		if(aw_valid_i.read())
			aw_ready_o.write(false);
		if(w_valid_i.read())
			w_ready_o.write(false);

		if (ar_valid_i.read()) {
			unsigned id = ar_id_i.read();
			auto it = active_transactions.find(id);
			if (it == active_transactions.end()) {
				payload_type* trans = tlm::tlm_mm<axi::axi_protocol_types>::get().allocate<axi::axi4_extension>();
			    register_trans(id, *trans, tlm::BEGIN_REQ);
			}
			auto read_trans = get_active_trans(id);
			payload_type* payload = read_trans->payload;
		    auto ext = payload->get_extension<axi::axi4_extension>();

			auto addr = ar_addr_i.read();
			auto length   = ar_len_i.read();
			auto size     = 1 << ar_size_i.read();
			auto buf_size = size * (length+1);
			uint8_t* r_data_buf = new uint8_t[buf_size];

			payload->acquire();
			payload->set_address(addr);
			payload->set_data_length(buf_size);
			payload->set_streaming_width(buf_size);
			payload->set_command(tlm::TLM_READ_COMMAND);
			payload->set_data_ptr(r_data_buf);
		    ext->set_size(ar_size_i.read());
		    ext->set_length(length);
		    ext->set_burst(axi::into<axi::burst_e>(ar_burst_i.read().to_uint()));
		    ext->set_id(ar_id_i.read());
		    ext->set_exclusive(ar_lock_i.read());
		    ext->set_cache(ar_cache_i.read());
		    ext->set_prot(ar_prot_i.read());
		    ext->set_qos(ar_qos_i.read());
		    ext->set_region(ar_region_i.read());
			ar_ready_o.write(true);
			output_socket->nb_transport_fw(*payload, read_trans->phase, delay);
		}

		if(aw_valid_i.read()) {
			unsigned id = aw_id_i.read();

			auto it = active_transactions.find(id);
			if (it == active_transactions.end()) {
				payload_type* trans = tlm::tlm_mm<axi::axi_protocol_types>::get().allocate<axi::axi4_extension>();
			    register_trans(id, *trans, axi::BEGIN_PARTIAL_REQ);
			}
			auto write_trans = get_active_trans(id);
			aw_ready_o.write(true);
			payload_type* p = write_trans->payload;
		    auto ext = p->get_extension<axi::axi4_extension>();
			auto length = aw_len_i.read();
			auto addr = aw_addr_i.read();
			auto num_bytes = 1 << aw_size_i.read();
			auto buf_size = num_bytes * (length+1);
			uint8_t* w_data_buf = new uint8_t[buf_size];


			write_trans->payload->acquire();
			write_trans->payload->set_address(addr);
			write_trans->payload->set_data_length(buf_size);
			write_trans->payload->set_streaming_width(buf_size);
			write_trans->payload->set_command(tlm::TLM_WRITE_COMMAND);
			write_trans->payload->set_data_ptr(w_data_buf);
		    ext->set_size(aw_size_i.read());
		    ext->set_length(length);
		    ext->set_burst(axi::into<axi::burst_e>(aw_burst_i.read().to_uint()));
		    ext->set_id(aw_id_i.read());
		    ext->set_exclusive(aw_lock_i.read());
		    ext->set_cache(aw_cache_i.read());
		    ext->set_prot(aw_prot_i.read());
		    ext->set_qos(aw_qos_i.read());
		    ext->set_region(aw_region_i.read());
		}
		if(w_valid_i.read()){
			unsigned id = aw_id_i.read();
			auto write_trans = get_active_trans(id);
			w_ready_o.write(true);
			write_data = w_data_i.read();
			auto num_bytes = 1 << aw_size_i.read();
			write_trans->payload->set_byte_enable_length(w_strb_i.read());
			auto data_ptr  = write_trans->payload->get_data_ptr();
			for (size_t i=0, j = 0; i < 8; j += num_bytes, i++) {
				data_ptr[i] = write_data.range(j + num_bytes - 1, j).to_uint64();
			}

			if (w_last_i.read()) {
				write_trans->phase = tlm::BEGIN_REQ;
				b_resp_o.write(axi::to_int(axi::resp_e::OKAY));
				b_valid_o.write(true);
			}
			output_socket->nb_transport_fw(*write_trans->payload, write_trans->phase, delay);
		}
    }
}

template <unsigned int BUSWIDTH, unsigned int ADDRWIDTH, unsigned int IDWIDTH>
inline tlm::tlm_sync_enum axi_pin2tlm_adaptor<BUSWIDTH, ADDRWIDTH, IDWIDTH>::axi_pin2tlm_adaptor::nb_transport_bw(payload_type& trans, phase_type& phase, sc_core::sc_time& t) {
	auto id = axi::get_axi_id(trans);
    auto active_trans = get_active_trans(id);
	SCCTRACE(SCMOD) << "Enter bw status: " << phase << " of "<<(trans.is_read()?"RD":"WR")<<" trans (axi_id:"<<id<<")";
	tlm::tlm_sync_enum status{tlm::TLM_ACCEPTED};
	if(trans.is_read()){
	    sc_dt::sc_biguint<BUSWIDTH> read_beat{0};
	    constexpr auto buswidth_in_bytes = BUSWIDTH / 8;
		axi::axi4_extension* ext;
		trans.get_extension(ext);
		sc_assert(ext && "axi4_extension missing");

		if ((phase == axi::BEGIN_PARTIAL_RESP && ext->get_length()>1) || phase == tlm::BEGIN_RESP) { // send a single beat

			phase = (phase == axi::BEGIN_PARTIAL_RESP) ? axi::END_PARTIAL_RESP : tlm::END_RESP;

			for(size_t i = 0, j = 0; j < ext->get_size(); i += 8, j++) {
				read_beat.range(i + 7, i) = *(uint8_t*)(trans.get_data_ptr() + j);
			}

			r_id_o.write(ext->get_id());
			r_resp_o.write(axi::to_int(ext->get_resp()));
			r_data_o.write(read_beat);
			r_valid_o.write(true);

			status = tlm::TLM_UPDATED;
			trans.set_address(trans.get_address() + buswidth_in_bytes);

			// EDN_RESP indicates the last phase of the AXI Read transaction
			if(phase == tlm::END_RESP) {
				r_last_o.write(true);
				active_trans->payload->release();
				active_transactions.erase(id);
			}
		} else if (phase == axi::BEGIN_PARTIAL_RESP ) { //TODO: clarify strange behavior of the simple_target: why does it always start with PARTIAL_RESP?
			phase = axi::END_PARTIAL_RESP;
			status = tlm::TLM_UPDATED;
		}
	} else { // WRITE transection
		if (phase == tlm::BEGIN_RESP) {
			phase = tlm::END_RESP;
			status = tlm::TLM_UPDATED;
			active_transactions.erase(id);
		}
	}
	active_trans->phase= phase;
	SCCTRACE(SCMOD) << "Exit bw status: " << phase << " of "<<(trans.is_read()?"RD":"WR")<<" trans (axi_id:"<<id<<")";

	return status;
}

template <unsigned int BUSWIDTH, unsigned int ADDRWIDTH, unsigned int IDWIDTH>
inline void axi_pin2tlm_adaptor<BUSWIDTH, ADDRWIDTH, IDWIDTH>::axi_pin2tlm_adaptor::reset() {
	SCCTRACE(SCMOD) << "Reset adapter";
    r_valid_o.write(false);
    r_last_o.write(false);
    b_valid_o.write(false);
    ar_ready_o.write(true);
    aw_ready_o.write(false);
}

template<unsigned int BUSWIDTH, unsigned int ADDRWIDTH, unsigned int IDWIDTH>
auto axi_pin2tlm_adaptor<BUSWIDTH, ADDRWIDTH, IDWIDTH>::get_active_trans(unsigned &id) {
	auto it = active_transactions.find(id);
	if (it == active_transactions.end())
		SCCERR(SCMOD) << "Invalid transaction ID " << id;

	auto trans = it->second;
	if (trans->payload == nullptr)
		SCCERR(SCMOD) << "Invalid transaction";

	return trans;
}

template<unsigned int BUSWIDTH, unsigned int ADDRWIDTH, unsigned int IDWIDTH>
void axi_pin2tlm_adaptor<BUSWIDTH, ADDRWIDTH, IDWIDTH>::register_trans(unsigned int axi_id, payload_type &trans, phase_type phase) {
	auto th = std::make_shared<trans_handle>();
	th->payload = &trans;
	th->phase = phase;
	active_transactions.insert(
			std::pair<const uint8_t, std::shared_ptr<trans_handle> >(axi_id,th));
}

} // namespace axi_bfm
