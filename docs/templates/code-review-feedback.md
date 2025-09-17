---
title: {filename}
description: Preserved review artifacts and rationale.
audience: [contributors]
domain: [quality]
tags: [review]
status: archive
---

# Code Review Feedback

| Date   | Agent   | SHA     | Branch   | Branch URL            | PR URL     |
| ------ | ------- | ------- | -------- | --------------------- | ---------- |
| {date} | {agent} | `{sha}` | {branch} | `{github_branch_url}` | `{pr_url}` |

## Instructions

Please carefully consider each of the following feedback items, collected from a GitHub code review.

Please act on each item by fixing the issue, or rejecting the feedback. Please update this document and fill out the information below each feedback item by replacing the text surrounded by curly braces. 

### Accepted Feedback Template

Please use the following template to record your acceptance.

```markdown

> [!NOTE]- **Accepted**
> | Confidence | Remarks |
> |------------|---------|
> | {confidence_score_out_of_10} | {confidence_rationale} |
>
> ## Lesson Learned
> 
> {lesson}
>
> ## What did you do to address this feedback?
>
> {what_you_did}
>
> ## Regression avoidance strategy
>
> {regression_avoidance_strategy}
>
> ## Notes
>
> {any_additional_context_or_say_none}

```

### Rejected Feedback Template

Please use the following template to record your rejections.

```markdown

> [!CAUTION]- **Rejected**
> | Confidence | Remarks |
> |------------|---------|
> | {confidence_score_out_of_10} | {confidence_rationale} |
>
> ## Rejection Rationale
>
> {rationale}
>
> ## What did you do instead?
>
> {what_you_did}
>
> ## Tradeoffs considered
>
> {pros_and_cons}
>
> ## What would make you change your mind
>
> {change_mind_conditions}
>
> ## Future Plans
>
> {future_plans}

```

---

## CODE REVIEW FEEDBACK

> How to use: copy one of the fenced templates above, paste under each feedback item, and replace all `{placeholders}`.

The following section contains the feedback items, extracted from the code review linked above. Please read each item and respond with your decision by injecting one of the two above templates beneath the feedback item.

### {feedback_item_title}

## Conclusion

| Accepted | Rejected | Remarks |
|-----------|----------|---------|
| {accepted_count} | {rejected_count} | {remarks} |

{comments}
