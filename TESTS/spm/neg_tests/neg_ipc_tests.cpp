/* Copyright (c) 2017 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "cmsis_os2.h"
#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "rtos.h"
#include "spm_client.h"
#include "psa_client_common.h"
#include "neg_tests.h"


#define MINOR_VER               5
#define CLIENT_RSP_BUF_SIZE     MBED_CONF_SPM_CLIENT_DATA_TX_BUF_SIZE_LIMIT
#define OFFSET_POS              1
#define INVALID_SFID            (PART1_SF1 + 30)
#define INVALID_MINOR           (MINOR_VER + 10)
#define INVALID_TX_LEN          (PSA_MAX_IOVEC_LEN + 1)


using namespace utest::v1;

Semaphore test_sem(0);
bool error_thrown = false;
extern "C" void spm_reboot(void);

void error(const char* format, ...)
{
    error_thrown = true;
    osStatus status = test_sem.release();
    MBED_ASSERT(status == osOK);
    while(1);
    PSA_UNUSED(status);
}

/* ------------------------------------- Functions ----------------------------------- */

static psa_handle_t client_ipc_tests_connect(uint32_t sfid, uint32_t minor_version)
{
    psa_handle_t handle = psa_connect( sfid,
                                       minor_version
                                     );

    TEST_ASSERT_TRUE(handle > 0);

    return handle;
}

static void client_ipc_tests_call( psa_handle_t handle,
                                   iovec *iovec_temp,
                                   size_t tx_len,
                                   size_t rx_len
                                 )
{
    error_t status = PSA_SUCCESS;
    uint8_t response_buf[CLIENT_RSP_BUF_SIZE] = {0};

    status = psa_call( handle,
                       iovec_temp,
                       tx_len,
                       response_buf,
                       rx_len
                     );

    TEST_ASSERT_EQUAL_INT(PSA_SUCCESS, status);
}

static void client_ipc_tests_close(psa_handle_t handle)
{
    error_t status = PSA_SUCCESS;
    status = psa_close(handle);

    TEST_ASSERT_EQUAL_INT(PSA_SUCCESS, status);
}

//Testing client call with an invalid SFID
void client_connect_invalid_sfid()
{
    psa_connect( INVALID_SFID,
                 MINOR_VER
               );

    TEST_FAIL_MESSAGE("client_connect_invalid_sfid negative test failed");
}

//Testing client connect version policy is RELAXED and minor version is bigger than the minimum version
void client_connect_invalid_pol_ver_relaxed()
{
    psa_connect( PART1_SF1,           //PART1_SF1 should have relaxed policy
                 INVALID_MINOR
               );

    TEST_FAIL_MESSAGE("client_connect_invalid_pol_ver_relaxed negative test failed");
}

//Testing client connect version policy is STRICT and minor version is different than the minimum version
void client_connect_invalid_pol_ver_strict()
{
    psa_connect( PART1_SF2,           //PART1_SF2 should have strict policy
                 INVALID_MINOR
               );

    TEST_FAIL_MESSAGE("client_connect_invalid_pol_ver_strict negative test failed");
}

//Testing client call num of iovec (tx_len) is bigger than max value allowed
void client_call_invalid_tx_len()
{
    psa_handle_t handle = 0;

    handle = client_ipc_tests_connect(PART1_SF1, MINOR_VER);

    uint8_t data[2] = {1, 0};

    iovec iovec_temp[PSA_MAX_IOVEC_LEN] = {{data, sizeof(data)},
                                           {data, sizeof(data)},
                                           {data, sizeof(data)}};

    client_ipc_tests_call(handle, iovec_temp, INVALID_TX_LEN, CLIENT_RSP_BUF_SIZE);

    TEST_FAIL_MESSAGE("client_call_invalid_tx_len negative test failed");
}

//Testing client call return buffer (rx_buff) is NULL and return buffer len is not 0
void client_call_rx_buff_null_rx_len_not_zero()
{
    psa_handle_t handle = 0;
    size_t rx_len = 1;

    handle = client_ipc_tests_connect(PART1_SF1, MINOR_VER);

    uint8_t data[2] = {1, 0};

    iovec iovec_temp[PSA_MAX_IOVEC_LEN] = {{data, sizeof(data)},
                                           {data, sizeof(data)},
                                           {data, sizeof(data)}};

    psa_call( handle,
              iovec_temp,
              PSA_MAX_IOVEC_LEN,
              NULL,
              rx_len
            );

    TEST_FAIL_MESSAGE("client_call_rx_buff_null_rx_len_not_zero negative test failed");
}

//Testing client call iovecs pointer is NULL and num of iovecs is not 0
void client_call_iovecs_null_tx_len_not_zero()
{
    psa_handle_t handle = 0;

    handle = client_ipc_tests_connect(PART1_SF1, MINOR_VER);

    uint8_t response_buf[CLIENT_RSP_BUF_SIZE] = {0};

    psa_call( handle,
              NULL,
              PSA_MAX_IOVEC_LEN,
              response_buf,
              CLIENT_RSP_BUF_SIZE
            );

    TEST_FAIL_MESSAGE("client_call_iovecs_null_tx_len_not_zero negative test failed");
}

//Testing client call iovecs total len is bigger than max value allowed
void client_call_iovec_total_len_exceeds_max_len()
{
    psa_handle_t handle = 0;

    handle = client_ipc_tests_connect(PART1_SF1, MINOR_VER);

    uint8_t data[MBED_CONF_SPM_CLIENT_DATA_TX_BUF_SIZE_LIMIT] = {1, 0};

    iovec iovec_temp[PSA_MAX_IOVEC_LEN] = {{data, sizeof(data)},
                                           {data, sizeof(data)},
                                           {data, sizeof(data)}};

    client_ipc_tests_call(handle, iovec_temp, PSA_MAX_IOVEC_LEN, 0);

    TEST_FAIL_MESSAGE("client_call_iovec_total_len_exceeds_max_len negative test failed");
}

//Testing client call iovec base 
void client_call_iovec_base_null_len_not_zero()
{
    client_ipc_tests_connect(PART1_SF1, MINOR_VER);

    uint8_t data[2] = {1, 0};

    iovec iovec_temp[PSA_MAX_IOVEC_LEN] = {{NULL, sizeof(data)},
                                           {data, sizeof(data)},
                                           {data, sizeof(data)}};

    client_ipc_tests_call(PSA_NULL_HANDLE, iovec_temp, PSA_MAX_IOVEC_LEN, 0);

    TEST_FAIL_MESSAGE("client_call_iovec_base_null_len_not_zero negative test failed");
}

//Testing client call handle does not exist on the platform
void client_call_invalid_handle()
{
    psa_handle_t handle = 0, invalid_handle = 0;

    handle = client_ipc_tests_connect(PART1_SF1, MINOR_VER);
    invalid_handle = handle + 10;

    uint8_t data[2] = {1, 0};

    iovec iovec_temp[PSA_MAX_IOVEC_LEN] = {{data, sizeof(data)},
                                           {data, sizeof(data)},
                                           {data, sizeof(data)}};

    client_ipc_tests_call(invalid_handle, iovec_temp, PSA_MAX_IOVEC_LEN, 0);

    TEST_FAIL_MESSAGE("client_call_invalid_handle negative test failed");
}

//Testing client call handle is null (PSA_NULL_HANDLE)
void client_call_handle_is_null()
{
    client_ipc_tests_connect(PART1_SF1, MINOR_VER);

    uint8_t data[2] = {1, 0};

    iovec iovec_temp[PSA_MAX_IOVEC_LEN] = {{data, sizeof(data)},
                                           {data, sizeof(data)},
                                           {data, sizeof(data)}};

    client_ipc_tests_call(PSA_NULL_HANDLE, iovec_temp, PSA_MAX_IOVEC_LEN, 0);

    TEST_FAIL_MESSAGE("client_call_handle_is_null negative test failed");
}

//Testing client close handle does not exist on the platform
void client_close_invalid_handle()
{
    psa_handle_t handle = 0, invalid_handle = 0;

    handle = client_ipc_tests_connect(PART1_SF1, MINOR_VER);
    invalid_handle = handle + 10;

    uint8_t data[2] = {1, 0};

    iovec iovec_temp[PSA_MAX_IOVEC_LEN] = {{data, sizeof(data)},
                                           {data, sizeof(data)},
                                           {data, sizeof(data)}};

    client_ipc_tests_call(handle, iovec_temp, PSA_MAX_IOVEC_LEN, 0);

    client_ipc_tests_close(invalid_handle);

    TEST_FAIL_MESSAGE("client_close_invalid_handle negative test failed");
}

//Testing server interrupt mask contains only a subset of interrupt signal mask
void server_interrupt_mask_invalid()
{
    psa_connect( PART2_INT_MASK,
                 MINOR_VER
               );

    TEST_FAIL_MESSAGE("server_interrupt_mask_invalid negative test failed at client side");
}

//Testing server get with msg NULL
void server_get_msg_null()
{
    psa_connect( PART2_GET_MSG_NULL,
                 MINOR_VER
               );

    TEST_FAIL_MESSAGE("server_get_msg_null negative test failed at client side");
}

//Testing server get signum have more than one bit ON
void server_get_multiple_bit_signum()
{
    psa_connect( PART2_GET_SIGNUM_MULTIPLE_BIT,
                 MINOR_VER
               );

    TEST_FAIL_MESSAGE("server_get_multiple_bit_signum negative test failed at client side");
}

//Testing server get signum is not a subset of current partition flags
void server_get_signum_not_subset()
{
    psa_connect( PART2_GET_SIGNUM_NOT_SUBSET,
                 MINOR_VER
               );

    TEST_FAIL_MESSAGE("server_get_signum_not_subset negative test failed at client side");
}

//Testing server get signum flag is not active
void server_get_signum_not_active()
{
    psa_connect( PART2_GET_SIGNUM_NOT_ACTIVE,
                 MINOR_VER
               );

    TEST_FAIL_MESSAGE("server_get_signum_not_active negative test failed at client side");
}

//Testing server read handle does not exist on the platform
void server_read_invalid_handle()
{
    psa_handle_t handle = 0;

    handle = client_ipc_tests_connect(PART2_READ_INVALID_HANDLE, MINOR_VER);

    client_ipc_tests_call(handle, NULL, 0, 0);

    TEST_FAIL_MESSAGE("server_read_invalid_handle negative test failed at client side");
}

//Testing server read handle is null (PSA_NULL_HANDLE)
void server_read_null_handle()
{
    psa_handle_t handle = 0;

    handle = client_ipc_tests_connect(PART2_READ_NULL_HANDLE, MINOR_VER);

    client_ipc_tests_call(handle, NULL, 0, 0);

    TEST_FAIL_MESSAGE("server_read_null_handle negative test failed at client side");
}

//Testing server read tx_size is 0
void server_read_tx_size_zero()
{
    psa_handle_t handle = 0;

    handle = client_ipc_tests_connect(PART2_READ_TX_SIZE_ZERO, MINOR_VER);

    client_ipc_tests_call(handle, NULL, 0, 0);

    TEST_FAIL_MESSAGE("server_read_tx_size_zero negative test failed at client side");
}

//Testing server write buffer is null
void server_write_null_buffer()
{
    psa_handle_t handle = 0;

    handle = client_ipc_tests_connect(PART2_WRITE_BUFFER_NULL, MINOR_VER);

    uint8_t data[2] = {1, 0};
    iovec iovec_temp[PSA_MAX_IOVEC_LEN] = {{data, sizeof(data)},
                                           {data, sizeof(data)},
                                           {data, sizeof(data)}};

    client_ipc_tests_call(handle, iovec_temp, PSA_MAX_IOVEC_LEN, CLIENT_RSP_BUF_SIZE);

    TEST_FAIL_MESSAGE("server_write_null_buffer negative test failed at client side");
}

//Testing server write offset bigger than max value
void server_write_offset_bigger_than_max_value()
{
    psa_handle_t handle = 0;

    handle = client_ipc_tests_connect(PART2_WRITE_OFFSET_BIGGER_MAX, MINOR_VER);

    uint8_t data[2] = {1, 0};
    iovec iovec_temp[PSA_MAX_IOVEC_LEN] = {{data, sizeof(data)},
                                           {data, sizeof(data)},
                                           {data, sizeof(data)}};
    
    client_ipc_tests_call(handle, iovec_temp, PSA_MAX_IOVEC_LEN, CLIENT_RSP_BUF_SIZE);

    TEST_FAIL_MESSAGE("server_write_offset_bigger_than_max_value negative test failed at client side");
}

//Testing server write offset bigger than rx_size
void server_write_offset_bigger_than_rx_size()
{
    psa_handle_t handle = 0;

    handle = client_ipc_tests_connect(PART2_WRITE_OFFSET_BIGGER_RX_SIZE, MINOR_VER);

    uint8_t data[2] = {1, 0};
    iovec iovec_temp[PSA_MAX_IOVEC_LEN] = {{data, sizeof(data)},
                                           {data, sizeof(data)},
                                           {data, sizeof(data)}};

    client_ipc_tests_call(handle, iovec_temp, PSA_MAX_IOVEC_LEN, CLIENT_RSP_BUF_SIZE);

    TEST_FAIL_MESSAGE("server_write_offset_bigger_than_rx_size negative test failed at client side");
}

//Testing server write rx_buff is null
void server_write_rx_buff_null()
{
    psa_handle_t handle = 0;

    handle = client_ipc_tests_connect(PART2_WRITE_RX_BUFF_NULL, MINOR_VER);

    uint8_t data[2] = {1, 0};
    iovec iovec_temp[PSA_MAX_IOVEC_LEN] = {{data, sizeof(data)},
                                           {data, sizeof(data)},
                                           {data, sizeof(data)}};

    client_ipc_tests_call(handle, iovec_temp, PSA_MAX_IOVEC_LEN, 0);

    TEST_FAIL_MESSAGE("server_write_rx_buff_null negative test failed at client side");
}

//Testing server write handle does not exist on the platform
void server_write_invalid_handle()
{
    psa_handle_t handle = 0;

    handle = client_ipc_tests_connect(PART2_WRITE_INVALID_HANDLE, MINOR_VER);

    uint8_t data[2] = {1, 0};
    iovec iovec_temp[PSA_MAX_IOVEC_LEN] = {{data, sizeof(data)},
                                           {data, sizeof(data)},
                                           {data, sizeof(data)}};

    client_ipc_tests_call(handle, iovec_temp, PSA_MAX_IOVEC_LEN, CLIENT_RSP_BUF_SIZE);

    TEST_FAIL_MESSAGE("server_write_invalid_handle negative test failed at client side");
}

//Testing server write handle is null (PSA_NULL_HANDLE)
void server_write_null_handle()
{
    psa_handle_t handle = 0;

    handle = client_ipc_tests_connect(PART2_WRITE_NULL_HANDLE, MINOR_VER);

    uint8_t data[2] = {1, 0};
    iovec iovec_temp[PSA_MAX_IOVEC_LEN] = {{data, sizeof(data)},
                                           {data, sizeof(data)},
                                           {data, sizeof(data)}};

    client_ipc_tests_call(handle, iovec_temp, PSA_MAX_IOVEC_LEN, CLIENT_RSP_BUF_SIZE);

    TEST_FAIL_MESSAGE("server_write_null_handle negative test failed at client side");
}

//Testing server end handle does not exist on the platform
void server_end_invalid_handle()
{
    psa_connect( PART2_END_INVALID_HANDLE,
                 MINOR_VER
               );
    
    TEST_FAIL_MESSAGE("server_end_invalid_handle negative test failed at client side");
}

//Testing server end handle is null (PSA_NULL_HANDLE)
void server_end_null_handle()
{
    psa_connect( PART2_END_NULL_HANDLE,
                 MINOR_VER
               );

    TEST_FAIL_MESSAGE("server_end_null_handle negative test failed at client side");
}

//Testing server end rhandle is not NULL and msg type is disconnect
void server_set_rhandle_invalid()
{
    psa_handle_t handle = 0;

    handle = client_ipc_tests_connect(PART2_SET_RHANDLE_INVALID, MINOR_VER);

    psa_close(handle);

    TEST_FAIL_MESSAGE("server_set_rhandle_invalid negative test failed at client side");
}

//Testing server notify partition id doesnt exist
void server_notify_part_id_invalid()
{
    psa_connect( PART2_NOTIFY_PART_ID_INVALID,
                 MINOR_VER
               );
    
    TEST_FAIL_MESSAGE("server_notify_part_id_invalid negative test failed at client side");
}

//Testing server psa_identity handle does not exist on the platform
void server_psa_identity_invalid_handle()
{
    psa_connect( PART2_IDENTITY_INVALID_HANDLE,
                 MINOR_VER
               );
    
    TEST_FAIL_MESSAGE("server_psa_identity_invalid_handle negative test failed at client side");
}

//Testing server psa_identity handle is null (PSA_NULL_HANDLE)
void server_psa_identity_null_handle()
{
    psa_connect( PART2_IDENTITY_NULL_HANDLE,
                 MINOR_VER
               );

    TEST_FAIL_MESSAGE("server_psa_identity_null_handle negative test failed at client side");
}

//Testing server psa_set_rhandle handle does not exist on the platform
void server_set_rhandle_invalid_handle()
{
    psa_connect( PART2_SET_RHANDLE_INVALID_HANDLE,
                 MINOR_VER
               );
    
    TEST_FAIL_MESSAGE("server_set_rhandle_invalid_handle negative test failed at client side");
}

//Testing server psa_set_rhandle handle is null (PSA_NULL_HANDLE)
void server_set_rhandle_part_id_invalid()
{
    psa_connect( PART2_SET_RHANDLE_NULL_HANDLE,
                 MINOR_VER
               );

    TEST_FAIL_MESSAGE("server_set_rhandle_part_id_invalid negative test failed at client side");
}

PSA_NEG_TEST(client_connect_invalid_sfid)
PSA_NEG_TEST(client_connect_invalid_pol_ver_relaxed)
PSA_NEG_TEST(client_connect_invalid_pol_ver_strict)
PSA_NEG_TEST(client_call_invalid_tx_len)
PSA_NEG_TEST(client_call_rx_buff_null_rx_len_not_zero)
PSA_NEG_TEST(client_call_iovecs_null_tx_len_not_zero)
PSA_NEG_TEST(client_call_iovec_total_len_exceeds_max_len)
PSA_NEG_TEST(client_call_iovec_base_null_len_not_zero)
PSA_NEG_TEST(client_call_invalid_handle)
PSA_NEG_TEST(client_call_handle_is_null)
PSA_NEG_TEST(client_close_invalid_handle)
PSA_NEG_TEST(server_interrupt_mask_invalid)
PSA_NEG_TEST(server_get_msg_null)
PSA_NEG_TEST(server_get_multiple_bit_signum)
PSA_NEG_TEST(server_get_signum_not_subset)
PSA_NEG_TEST(server_get_signum_not_active)
PSA_NEG_TEST(server_read_invalid_handle)
PSA_NEG_TEST(server_read_null_handle)
PSA_NEG_TEST(server_read_tx_size_zero)
PSA_NEG_TEST(server_write_null_buffer)
PSA_NEG_TEST(server_write_offset_bigger_than_max_value)
PSA_NEG_TEST(server_write_offset_bigger_than_rx_size)
PSA_NEG_TEST(server_write_rx_buff_null)
PSA_NEG_TEST(server_write_invalid_handle)
PSA_NEG_TEST(server_write_null_handle)
PSA_NEG_TEST(server_end_invalid_handle)
PSA_NEG_TEST(server_end_null_handle)
PSA_NEG_TEST(server_set_rhandle_invalid)
PSA_NEG_TEST(server_notify_part_id_invalid)
PSA_NEG_TEST(server_psa_identity_invalid_handle)
PSA_NEG_TEST(server_psa_identity_null_handle)
PSA_NEG_TEST(server_set_rhandle_invalid_handle)
PSA_NEG_TEST(server_set_rhandle_part_id_invalid)


utest::v1::status_t spm_case_teardown(const Case *const source, const size_t passed, const size_t failed, const failure_t reason)
{
    spm_reboot();
    error_thrown = false;
    return greentea_case_teardown_handler(source, passed, failed, reason);
}

// Test cases
Case cases[] = {
    Case("Testing client connect invalid sfid", PSA_NEG_TEST_NAME(client_connect_invalid_sfid), spm_case_teardown),
    Case("Testing client connect version policy relaxed invalid minor", PSA_NEG_TEST_NAME(client_connect_invalid_pol_ver_relaxed), spm_case_teardown),
    Case("Testing client connect version policy strict invalid minor", PSA_NEG_TEST_NAME(client_connect_invalid_pol_ver_strict), spm_case_teardown),
    Case("Testing client call invalid tx_len", PSA_NEG_TEST_NAME(client_call_invalid_tx_len), spm_case_teardown),
    Case("Testing client call rx_buff is NULL rx_len is not 0", PSA_NEG_TEST_NAME(client_call_rx_buff_null_rx_len_not_zero), spm_case_teardown),
    Case("Testing client call iovecs is NULL tx_len is not 0", PSA_NEG_TEST_NAME(client_call_iovecs_null_tx_len_not_zero), spm_case_teardown),
    Case("Testing client call iovecs total len exceeds max len", PSA_NEG_TEST_NAME(client_call_iovec_total_len_exceeds_max_len), spm_case_teardown),
    Case("Testing client call iovec base NULL while iovec len not 0", PSA_NEG_TEST_NAME(client_call_iovec_base_null_len_not_zero), spm_case_teardown),
    Case("Testing client call handle does not exist", PSA_NEG_TEST_NAME(client_call_invalid_handle), spm_case_teardown),
    Case("Testing client call handle is PSA_NULL_HANDLE", PSA_NEG_TEST_NAME(client_call_handle_is_null), spm_case_teardown),
    Case("Testing client close handle does not exist", PSA_NEG_TEST_NAME(client_close_invalid_handle), spm_case_teardown),
    Case("Testing server interrupt mask invalid", PSA_NEG_TEST_NAME(server_interrupt_mask_invalid), spm_case_teardown),
    Case("Testing server get with msg NULL", PSA_NEG_TEST_NAME(server_get_msg_null), spm_case_teardown),
    Case("Testing server get signum have more than one bit ON", PSA_NEG_TEST_NAME(server_get_multiple_bit_signum), spm_case_teardown),
    Case("Testing server get signum flag is not a subset of current partition flags", PSA_NEG_TEST_NAME(server_get_signum_not_subset), spm_case_teardown),
    Case("Testing server get signum flag is not active", PSA_NEG_TEST_NAME(server_get_signum_not_active), spm_case_teardown),
    Case("Testing server read handle does not exist on the platform", PSA_NEG_TEST_NAME(server_read_invalid_handle), spm_case_teardown),
    Case("Testing server read handle is PSA_NULL_HANDLE", PSA_NEG_TEST_NAME(server_read_null_handle), spm_case_teardown),
    Case("Testing server read tx_size is 0", PSA_NEG_TEST_NAME(server_read_tx_size_zero), spm_case_teardown),
    Case("Testing server write buffer is NULL", PSA_NEG_TEST_NAME(server_write_null_buffer), spm_case_teardown),
    Case("Testing server write offset bigger than max value", PSA_NEG_TEST_NAME(server_write_offset_bigger_than_max_value), spm_case_teardown),
    Case("Testing server write offset bigger than rx_size", PSA_NEG_TEST_NAME(server_write_offset_bigger_than_rx_size), spm_case_teardown),
    Case("Testing server write rx_buff is null", PSA_NEG_TEST_NAME(server_write_rx_buff_null), spm_case_teardown),
    Case("Testing server write handle does not exist on the platform", PSA_NEG_TEST_NAME(server_write_invalid_handle), spm_case_teardown),
    Case("Testing server write handle is PSA_NULL_HANDLE", PSA_NEG_TEST_NAME(server_write_null_handle), spm_case_teardown),
    Case("Testing server end handle does not exist on the platform", PSA_NEG_TEST_NAME(server_end_invalid_handle), spm_case_teardown),
    Case("Testing server end handle is PSA_NULL_HANDLE", PSA_NEG_TEST_NAME(server_end_null_handle), spm_case_teardown),
    Case("Testing server set rhandle is not NULL while msg type is disconnect", PSA_NEG_TEST_NAME(server_set_rhandle_invalid), spm_case_teardown),
    Case("Testing server notify partition id doesnt exist", PSA_NEG_TEST_NAME(server_notify_part_id_invalid), spm_case_teardown),
    Case("Testing server identify handle does not exist on the platform", PSA_NEG_TEST_NAME(server_psa_identity_invalid_handle), spm_case_teardown),
    Case("Testing server identify handle is PSA_NULL_HANDLE", PSA_NEG_TEST_NAME(server_psa_identity_null_handle), spm_case_teardown),
    Case("Testing server set_rhandle handle does not exist on the platform", PSA_NEG_TEST_NAME(server_set_rhandle_invalid_handle), spm_case_teardown),
    Case("Testing server set_rhandle handle is PSA_NULL_HANDLE", PSA_NEG_TEST_NAME(server_set_rhandle_part_id_invalid), spm_case_teardown),
};

utest::v1::status_t spm_setup(const size_t number_of_cases) 
{
#ifndef NO_GREENTEA
    GREENTEA_SETUP(20, "default_auto");
#endif
    return greentea_test_setup_handler(number_of_cases);
}

Specification specification(spm_setup, cases);

int main()
{
    !Harness::run(specification);
    return 0;
}
