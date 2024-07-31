#include <check.h>
#include "proxy_parse.h" // Include your header files here

// Test case for fetch_url
START_TEST(test_fetch_url_success)
{
    char *url = "http://example.com";
    HttpResponse response;
    memset(&response, 0, sizeof(response));
    
    fetch_url(url, &response);

    ck_assert_ptr_ne(response.body, NULL);
    ck_assert_int_gt(response.body_len, 0);
    ck_assert_ptr_ne(response.headers, NULL);
    ck_assert_int_gt(response.headers_len, 0);

    free(response.body);
    free(response.headers);
}
END_TEST

// Test case for add_cache_element
START_TEST(test_add_cache_element)
{
    char *request = "http://example.com";
    HttpResponse response;
    memset(&response, 0, sizeof(response));
    response.body = strdup("response body");
    response.body_len = strlen(response.body);
    response.headers = strdup("response headers");
    response.headers_len = strlen(response.headers);

    int result = add_cache_element(request, strlen(request), &response);

    ck_assert_int_eq(result, 1);
    cache_element *cached = find(request);
    ck_assert_ptr_ne(cached, NULL);
    ck_assert_str_eq(cached->request, request);
    ck_assert_int_eq(cached->response->body_len, response.body_len);
    ck_assert_str_eq(cached->response->body, response.body);

    free(cached->response->body);
    free(cached->response->headers);
    free(cached->response);
    free(cached->request);
    free(cached);
}
END_TEST

// Main function to run all tests
int main(void)
{
    Suite *s;
    SRunner *sr;
    int number_failed;

    s = suite_create("Proxy Tests");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_fetch_url_success);
    tcase_add_test(tc_core, test_add_cache_element);
    suite_add_tcase(s, tc_core);

    sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
