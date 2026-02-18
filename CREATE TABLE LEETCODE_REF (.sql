CREATE TABLE LEETCODE_REF (
    PROB_ID NUMBER PRIMARY KEY,
    TITLE VARCHAR2(200),
    TAGS VARCHAR2(500)
);

INSERT INTO LEETCODE_REF VALUES (1, 'Two Sum', 'array, hashmap, target, sum');
INSERT INTO LEETCODE_REF VALUES (307, 'Range Sum Query', 'array, update, range, segment, sum');
INSERT INTO LEETCODE_REF VALUES (20, 'Valid Parentheses', 'stack, string, bracket, valid');

COMMIT;