#include "vector_unit.h"
#include "sim.h"
#include "cfg.h"

static sim_t *s = NULL;
static processor_t *p = NULL;
static state_t *state = NULL;
static vectorUnit_t *vu = NULL;

void sim_t::vector_unit_init()
{
  p = get_core("0");
  state = p->get_state();
  vu = &p->VU;
}

uint64_t sim_t::vector_unit_vreg_read(reg_t vReg, reg_t n) {
    uint64_t val = p->VU.elt<uint64_t>(vReg, n);
    return val;
}

void sim_t::vector_unit_vreg_write(reg_t vReg, reg_t n, uint64_t val) {
    auto &vd = p->VU.elt<uint64_t>(vReg, n, true);
    vd = val;
}

uint64_t sim_t::vector_unit_reg_read(reg_t Reg) {
    uint64_t val = state->XPR[Reg];
    return val;
}

void sim_t::vector_unit_reg_write(reg_t Reg, uint64_t val) {
    state->XPR.write(Reg, val);
}

void sim_t::vector_unit_execute_insn(uint64_t insn) {
    p->execute_insn(insn);
}

uint64_t sim_t::vector_unit_pc_read() {
    return state->pc;
}

void sim_t::vector_unit_set_print() {
    p->VU.print_vector_unit();
}

static std::vector<std::pair<reg_t, mem_t*>> make_vector_mems(const std::vector<mem_cfg_t> &layout)
{
    std::vector<std::pair<reg_t, mem_t*>> mems;
    mems.reserve(layout.size());
    for (const auto &cfg : layout) {
        mems.push_back(std::make_pair(cfg.base, new mem_t(cfg.size)));
    }
    return mems;
}

extern "C" {
    uint64_t vector_vreg_read(reg_t vReg, reg_t n) {
        return s->vector_unit_vreg_read(vReg, n);
    }

    void vector_vreg_write(reg_t vReg, reg_t n, uint64_t val) {
        s->vector_unit_vreg_write(vReg, n, val);
    }

    uint64_t vector_pc_read() {
        return s->vector_unit_pc_read();
    }

    uint64_t vector_reg_read(reg_t Reg) {
        return s->vector_unit_reg_read(Reg);
    }

    void vector_reg_write(reg_t Reg, uint64_t val) {
        s->vector_unit_reg_write(Reg, val);
    }

    void vector_execute_insn(uint64_t insn) {
        s->vector_unit_execute_insn(insn);
    }

    void vector_step(int n) {
        p->step(n);
    }

    void vector_set_print() {
        s->vector_unit_set_print();
    }

    void vector_sim_init() {
        std::vector<mem_cfg_t> mem_cfg { mem_cfg_t(0x80000000, 0x10000000) };
        std::vector<int> hartids = {0};
        auto const cfg = new cfg_t(std::make_pair(0, 0),
                nullptr,
                "rv64gv",
                "MSU",
                "vlen:128,elen:64",
                endianness_little,
                16,
                mem_cfg,
                hartids,
                false);
        std::vector<std::pair<reg_t, abstract_device_t*>> plugin_devices;
        std::vector<std::string> htif_args {"pk", "hello"};
        debug_module_config_t dm_config = {
            .progbufsize = 2,
            .max_sba_data_width = 0,
            .require_authentication = false,
            .abstract_rti = 0,
            .support_hasel = true,
            .support_abstract_csr_access = true,
            .support_abstract_fpr_access = true,
            .support_haltgroups = true,
            .support_impebreak = true
        };
        std::vector<std::pair<reg_t, mem_t*>> mems = make_vector_mems(cfg->mem_layout());
        s = new sim_t(cfg, false,
                mems,
                plugin_devices,
                htif_args,
                dm_config,
                nullptr,
                true,
                nullptr,
                false,
                nullptr);
        s->vector_unit_init();
        if (p->extension_enabled('V')) {
            printf("Vector extension enabled\n");
        }
        p->VU.reset();
    }

}
