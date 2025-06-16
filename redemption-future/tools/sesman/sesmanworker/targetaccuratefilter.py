"""

Filter field by field by applying one or several filters
on target string like account@domain@device:service

For active the Target Accurate Filter mode, user MUST type on
selector research a string form beginning by '?' character like :

  "?keyword1=value1&?keyword2=value2&?keyword3=value3"


About syntax, THREE SPECIAL characters are defined :
- '?' in front of a word is considered as keyword
- '&' between two filters is considered as filter separator
- '=' between keyword and value is considered as filter key-value separator

Each wanted filter must following the "?keyword=value"
format for be applicable.


For example, the following target filter string will filter on
"account", "domain", "device" and "service" keyword fields according to
the passed values :

  "?account=my_account&?domain=my_domain&?device=my_device&?service=my_service"


We can combine 2 filters or more like above but we
can also execute only one filter on target.
The following target filter strings are accepted :

  "?account=my_account"

  "?domain=my_domain"

  "?device=my_device"

  "?service=my_service"


There is no specifc order for apply filters.
The following target filter string is also valid :

  "?device=my_device&?account=my_account&?domain=my_domain"


BE CAREFUL, some peculiarities are possible about syntax.
The following target filter strings is considered as valid :

  "?account=&&to&to&&" (account value : "&&to&to&&")

  "?domain===to=to==" (domain value : "==to=to==")

  "?device=&=&to=to&=&=" (device value : "&=&to=to&=&=")

  "?service=&=&=&=&" (service value : "&=&=&=&")

  "?device=toto=tata&&?account=titi&" (device value : "toto=tata&",
                                       account value : "titi&")

The last one can be very misleading because device value isn't equal
to "toto=tata&&" but "toto=tata&" because '&' is both a special character
AND a valid character for target string !!!


However, target filter string badly formatted like containing
duplicated filter keywords, missing values and others aren't
allowed and will be skipped and considered as no valid.
The following examples are forbidden :

  "?account=my_account&?account=my_account"

  "?&domain=my_domain"

  "?my_device=my_device"

  "?service="

  "?account?&my_account"

  "?abcd=my_domain"


"""

from typing import Optional, List, Dict

FILTER_KEYWORD_LIST = (
    "account",
    "domain",
    "device",
    "service",
)

KEYWORD_PREFIX = '?'
FILTER_SEPARATOR = '&?'
FILTER_KV_SEPARATOR = '='


class SelectorFilterMode:
    NONE = 0
    NORMAL = 1
    ADVANCED = 2


class FilterKeywordSyntaxError(RuntimeError):
    def __init__(self, keyword):
        super().__init__(
            self, f"unknown filter keyword with '{keyword}'")


class FilterKeywordDuplicateError(RuntimeError):
    def __init__(self, keyword):
        super().__init__(
            self, f"duplicated filter keyword with '{keyword}'")


class ParsingError(RuntimeError):
    def __init__(self, bad_string):
        super().__init__(
            self,
            f"'keyword{FILTER_KV_SEPARATOR}value' format parsing error with '{bad_string}'")


def get_selector_filter_mode(pattern: str) -> int:
    if not pattern:
        return SelectorFilterMode.NONE
    elif not pattern.startswith(KEYWORD_PREFIX):
        return SelectorFilterMode.NORMAL
    else:
        return SelectorFilterMode.ADVANCED


def try_get_filter_keyword_without_kprefix(filter_keyword: str) -> Optional[str]:
    if filter_keyword.startswith(KEYWORD_PREFIX):
        filter_keyword_without_kprefix = filter_keyword[1:]

        if filter_keyword_without_kprefix in FILTER_KEYWORD_LIST:
            return filter_keyword_without_kprefix
    return None


def filter_patterns_splitting(filter_patterns: str) -> List[str]:
    word_list = filter_patterns.split(FILTER_SEPARATOR)
    return [KEYWORD_PREFIX + filter_pattern if i != 0 else filter_pattern
            for i, filter_pattern in enumerate(word_list)]


def get_filter_pattern_dict(filter_patterns: str) -> Dict[str, str]:
    filter_pattern_dict = {}

    for filter_kv in filter_patterns_splitting(filter_patterns):
        token_list = filter_kv.split(FILTER_KV_SEPARATOR, 1)

        if len(token_list) != 2:
            raise ParsingError(filter_kv)

        filter_keyword, filter_value = token_list

        if not filter_value:
            raise ParsingError(filter_kv)

        filter_keyword_without_kprefix = (
            try_get_filter_keyword_without_kprefix(filter_keyword))

        if not filter_keyword_without_kprefix:
            raise FilterKeywordSyntaxError(filter_keyword)
        if filter_keyword_without_kprefix not in filter_pattern_dict:
            filter_pattern_dict[
                filter_keyword_without_kprefix] = filter_value
        else:
            raise FilterKeywordDuplicateError(filter_keyword)
    return filter_pattern_dict


def is_filterable(filter_pattern_dict: Dict[str, str],
                  target_field_dict: Dict[str, str]) -> bool:
    return all(kw in target_field_dict and value in target_field_dict[kw]
               for kw, value in filter_pattern_dict.items())
