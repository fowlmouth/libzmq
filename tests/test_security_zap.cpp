/*
    Copyright (c) 2007-2017 Contributors as noted in the AUTHORS file

    This file is part of libzmq, the ZeroMQ core engine in C++.

    libzmq is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, the Contributors give you permission to link
    this library with independent modules to produce an executable,
    regardless of the license terms of these independent modules, and to
    copy and distribute the resulting executable under terms of your choice,
    provided that you also meet, for each linked independent module, the
    terms and conditions of the license of that module. An independent
    module is a module which is not derived from or based on this library.
    If you modify this library, you must extend this exception to your
    version of the library.

    libzmq is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "testutil_security.hpp"

static void zap_handler_wrong_version (void *ctx)
{
    zap_handler_generic (ctx, zap_wrong_version);
}

static void zap_handler_wrong_request_id (void *ctx)
{
    zap_handler_generic (ctx, zap_wrong_request_id);
}

static void zap_handler_wrong_status_invalid (void *ctx)
{
    zap_handler_generic (ctx, zap_status_invalid);
}

static void zap_handler_wrong_status_temporary_failure (void *ctx)
{
    zap_handler_generic (ctx, zap_status_temporary_failure);
}

static void zap_handler_wrong_status_internal_error (void *ctx)
{
    zap_handler_generic (ctx, zap_status_internal_error);
}

static void zap_handler_too_many_parts (void *ctx)
{
    zap_handler_generic (ctx, zap_too_many_parts);
}

static void zap_handler_disconnect (void *ctx)
{
    zap_handler_generic (ctx, zap_disconnect);
}

static void zap_handler_do_not_recv (void *ctx)
{
    zap_handler_generic (ctx, zap_do_not_recv);
}

static void zap_handler_do_not_send (void *ctx)
{
    zap_handler_generic (ctx, zap_do_not_send);
}

int expect_new_client_bounce_fail_and_count_monitor_events (
  void *ctx,
  char *my_endpoint,
  void *server,
  socket_config_fn socket_config_,
  void *socket_config_data_,
  void **client_mon,
  void *server_mon,
  int expected_server_event,
  int expected_server_value,
  int expected_client_event = 0,
  int expected_client_value = 0)
{
    expect_new_client_bounce_fail (
      ctx, my_endpoint, server, socket_config_, socket_config_data_, client_mon,
      expected_client_event, expected_client_value);

    int events_received = 0;
#ifdef ZMQ_BUILD_DRAFT_API
    events_received = expect_monitor_event_multiple (
      server_mon, expected_server_event, expected_server_value);
#endif

    return events_received;
}

void test_zap_unsuccessful (void *ctx,
                            char *my_endpoint,
                            void *server,
                            void *server_mon,
                            int expected_server_event,
                            int expected_server_value,
                            socket_config_fn socket_config_,
                            void *socket_config_data_,
                            void **client_mon = NULL,
                            int expected_client_event = 0,
                            int expected_client_value = 0)
{
    int server_events_received =
      expect_new_client_bounce_fail_and_count_monitor_events (
        ctx, my_endpoint, server, socket_config_, socket_config_data_,
        client_mon, server_mon, expected_server_event, expected_server_value,
        expected_client_event, expected_client_value);

    //  there may be more than one ZAP request due to repeated attempts by the
    //  client (actually only in case if ZAP status code 300)
    assert (server_events_received == 0
            || 1 <= zmq_atomic_counter_value (zap_requests_handled));
}

void test_zap_unsuccessful_no_handler (void *ctx,
                                       char *my_endpoint,
                                       void *server,
                                       void *server_mon,
                                       int expected_event,
                                       int expected_err,
                                       socket_config_fn socket_config_,
                                       void *socket_config_data_,
                                       void **client_mon = NULL)
{
    int events_received =
      expect_new_client_bounce_fail_and_count_monitor_events (
        ctx, my_endpoint, server, socket_config_, socket_config_data_,
        client_mon, server_mon, expected_event, expected_err);

#ifdef ZMQ_BUILD_DRAFT_API
    //  there may be more than one ZAP request due to repeated attempts by the
    //  client
    assert (events_received > 0);
#else
    LIBZMQ_UNUSED (events_received);
#endif
}

void test_zap_protocol_error (void *ctx,
                              char *my_endpoint,
                              void *server,
                              void *server_mon,
                              socket_config_fn socket_config_,
                              void *socket_config_data_,
                              int expected_error)
{
    test_zap_unsuccessful (ctx, my_endpoint, server, server_mon,
#ifdef ZMQ_BUILD_DRAFT_API
                           ZMQ_EVENT_HANDSHAKE_FAILED_PROTOCOL, expected_error,
#else
                           0, 0,
#endif
                           socket_config_, socket_config_data_);
}

void test_zap_unsuccessful_status_300 (void *ctx,
                                       char *my_endpoint,
                                       void *server,
                                       void *server_mon,
                                       socket_config_fn client_socket_config_,
                                       void *client_socket_config_data_)
{
    void *client_mon;
    test_zap_unsuccessful (ctx, my_endpoint, server, server_mon,
#ifdef ZMQ_BUILD_DRAFT_API
                           ZMQ_EVENT_HANDSHAKE_FAILED_AUTH, 300,
#else
                           0, 0,
#endif
                           client_socket_config_, client_socket_config_data_,
                           &client_mon);

#ifdef ZMQ_BUILD_DRAFT_API
    // we can use a 0 timeout here, since the client socket is already closed
    assert_no_more_monitor_events_with_timeout (client_mon, 0);

    int rc = zmq_close (client_mon);
    assert (rc == 0);
#endif
}

void test_zap_unsuccessful_status_500 (void *ctx,
                                       char *my_endpoint,
                                       void *server,
                                       void *server_mon,
                                       socket_config_fn client_socket_config_,
                                       void *client_socket_config_data_)
{
    test_zap_unsuccessful (ctx, my_endpoint, server, server_mon,
#ifdef ZMQ_BUILD_DRAFT_API
                           ZMQ_EVENT_HANDSHAKE_FAILED_AUTH, 500,
#else
                           0, 0,
#endif
                           client_socket_config_, client_socket_config_data_,
                           NULL,
#ifdef ZMQ_BUILD_DRAFT_API
                           ZMQ_EVENT_HANDSHAKE_FAILED_AUTH, 500
#else
                           0, 0
#endif
    );
}

void test_zap_errors (socket_config_fn server_socket_config_,
                      void *server_socket_config_data_,
                      socket_config_fn client_socket_config_,
                      void *client_socket_config_data_)
{
    void *ctx;
    void *handler;
    void *zap_thread;
    void *server;
    void *server_mon;
    char my_endpoint[MAX_SOCKET_STRING];

    //  Invalid ZAP protocol tests

    //  wrong version
    fprintf (stderr, "test_zap_protocol_error wrong_version\n");
    setup_context_and_server_side (
      &ctx, &handler, &zap_thread, &server, &server_mon, my_endpoint,
      &zap_handler_wrong_version, server_socket_config_,
      server_socket_config_data_);
    test_zap_protocol_error (ctx, my_endpoint, server, server_mon,
                             client_socket_config_, client_socket_config_data_,
#ifdef ZMQ_BUILD_DRAFT_API
                             ZMQ_PROTOCOL_ERROR_ZAP_BAD_VERSION
#else
                             0
#endif
    );
    shutdown_context_and_server_side (ctx, zap_thread, server, server_mon,
                                      handler);

    //  wrong request id
    fprintf (stderr, "test_zap_protocol_error wrong_request_id\n");
    setup_context_and_server_side (
      &ctx, &handler, &zap_thread, &server, &server_mon, my_endpoint,
      &zap_handler_wrong_request_id, server_socket_config_,
      server_socket_config_data_);
    test_zap_protocol_error (ctx, my_endpoint, server, server_mon,
                             client_socket_config_, client_socket_config_data_,
#ifdef ZMQ_BUILD_DRAFT_API
                             ZMQ_PROTOCOL_ERROR_ZAP_BAD_REQUEST_ID
#else
                             0
#endif
    );
    shutdown_context_and_server_side (ctx, zap_thread, server, server_mon,
                                      handler);

    //  status invalid (not a 3-digit number)
    fprintf (stderr, "test_zap_protocol_error wrong_status_invalid\n");
    setup_context_and_server_side (
      &ctx, &handler, &zap_thread, &server, &server_mon, my_endpoint,
      &zap_handler_wrong_status_invalid, server_socket_config_,
      server_socket_config_data_);
    test_zap_protocol_error (ctx, my_endpoint, server, server_mon,
                             client_socket_config_, client_socket_config_data_,
#ifdef ZMQ_BUILD_DRAFT_API
                             ZMQ_PROTOCOL_ERROR_ZAP_INVALID_STATUS_CODE
#else
                             0
#endif
    );
    shutdown_context_and_server_side (ctx, zap_thread, server, server_mon,
                                      handler);

    //  too many parts
    fprintf (stderr, "test_zap_protocol_error too_many_parts\n");
    setup_context_and_server_side (
      &ctx, &handler, &zap_thread, &server, &server_mon, my_endpoint,
      &zap_handler_too_many_parts, server_socket_config_,
      server_socket_config_data_);
    test_zap_protocol_error (ctx, my_endpoint, server, server_mon,
                             client_socket_config_, client_socket_config_data_,
#ifdef ZMQ_BUILD_DRAFT_API
                             ZMQ_PROTOCOL_ERROR_ZAP_MALFORMED_REPLY
#else
                             0
#endif
    );
    shutdown_context_and_server_side (ctx, zap_thread, server, server_mon,
                                      handler);

    //  ZAP non-standard cases

    //  TODO make these observable on the client side as well (they are
    //  transmitted as an ERROR message)

    //  status 300 temporary failure
    fprintf (stderr, "test_zap_unsuccessful status 300\n");
    setup_context_and_server_side (
      &ctx, &handler, &zap_thread, &server, &server_mon, my_endpoint,
      &zap_handler_wrong_status_temporary_failure, server_socket_config_,
      server_socket_config_data_);
    test_zap_unsuccessful_status_300 (ctx, my_endpoint, server, server_mon,
                                      client_socket_config_,
                                      client_socket_config_data_);
    shutdown_context_and_server_side (ctx, zap_thread, server, server_mon,
                                      handler);

    //  status 500 internal error
    fprintf (stderr, "test_zap_unsuccessful status 500\n");
    setup_context_and_server_side (
      &ctx, &handler, &zap_thread, &server, &server_mon, my_endpoint,
      &zap_handler_wrong_status_internal_error, server_socket_config_);
    test_zap_unsuccessful_status_500 (ctx, my_endpoint, server, server_mon,
                                      client_socket_config_,
                                      client_socket_config_data_);
    shutdown_context_and_server_side (ctx, zap_thread, server, server_mon,
                                      handler);

#ifdef ZMQ_ZAP_ENFORCE_DOMAIN
    //  no ZAP handler
    fprintf (stderr, "test_zap_unsuccessful no ZAP handler started\n");
    setup_context_and_server_side (&ctx, &handler, &zap_thread, &server,
                                   &server_mon, my_endpoint, NULL,
                                   server_socket_config_);
    test_zap_unsuccessful_no_handler (
      ctx, my_endpoint, server, server_mon,
#ifdef ZMQ_BUILD_DRAFT_API
      ZMQ_EVENT_HANDSHAKE_FAILED_NO_DETAIL, EFAULT,
#else
      0, 0,
#endif
      client_socket_config_, client_socket_config_data_);
    shutdown_context_and_server_side (ctx, zap_thread, server, server_mon,
                                      handler);
#endif

    //  ZAP handler disconnecting on first message
    fprintf(stderr, "test_zap_unsuccessful ZAP handler disconnects\n");
    setup_context_and_server_side(&ctx, &handler, &zap_thread, &server,
        &server_mon, my_endpoint, &zap_handler_disconnect,
        server_socket_config_);
    test_zap_unsuccessful_no_handler (
      ctx, my_endpoint, server, server_mon,
#ifdef ZMQ_BUILD_DRAFT_API
      ZMQ_EVENT_HANDSHAKE_FAILED_NO_DETAIL, EPIPE,
#else
      0, 0,
#endif
      client_socket_config_, client_socket_config_data_);
    shutdown_context_and_server_side (ctx, zap_thread, server, server_mon,
                                      handler, true);

    //  ZAP handler does not read request
    fprintf (stderr,
             "test_zap_unsuccessful ZAP handler does not read request\n");
    setup_context_and_server_side (&ctx, &handler, &zap_thread, &server,
        &server_mon, my_endpoint, &zap_handler_do_not_recv,
        server_socket_config_);
    test_zap_unsuccessful_no_handler (
      ctx, my_endpoint, server, server_mon,
#ifdef ZMQ_BUILD_DRAFT_API
      ZMQ_EVENT_HANDSHAKE_FAILED_NO_DETAIL, EPIPE,
#else
      0, 0,
#endif
      client_socket_config_, client_socket_config_data_);
    shutdown_context_and_server_side (ctx, zap_thread, server, server_mon,
                                      handler);

    //  ZAP handler does not send reply
    fprintf (stderr,
             "test_zap_unsuccessful ZAP handler does not write reply\n");
    setup_context_and_server_side (
      &ctx, &handler, &zap_thread, &server, &server_mon, my_endpoint,
      &zap_handler_do_not_send, server_socket_config_);
    test_zap_unsuccessful_no_handler (
      ctx, my_endpoint, server, server_mon,
#ifdef ZMQ_BUILD_DRAFT_API
      ZMQ_EVENT_HANDSHAKE_FAILED_NO_DETAIL, EPIPE,
#else
      0, 0,
#endif
      client_socket_config_, client_socket_config_data_);
    shutdown_context_and_server_side (ctx, zap_thread, server, server_mon,
                                      handler);
}

int main (void)
{
    setup_test_environment ();

    fprintf (stderr, "NULL mechanism\n");
    test_zap_errors (&socket_config_null_server, NULL,
                     &socket_config_null_client, NULL);

    fprintf (stderr, "PLAIN mechanism\n");
    test_zap_errors (&socket_config_plain_server, NULL,
                     &socket_config_plain_client, NULL);

    if (zmq_has ("curve")) {
        fprintf (stderr, "CURVE mechanism\n");
        setup_testutil_security_curve ();

        curve_client_data_t curve_client_data = {
          valid_server_public, valid_client_public, valid_client_secret};
        test_zap_errors (&socket_config_curve_server, valid_server_secret,
                         &socket_config_curve_client, &curve_client_data);
    }
}
