from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
MAIN_PAGE = PROJECT_ROOT / "static" / "cloud_display.html"
PROTOTYPE_PAGE = PROJECT_ROOT.parent / "02_界面原型" / "cloud_display.html"


def test_cloud_display_copies_stay_identical():
    assert MAIN_PAGE.read_bytes() == PROTOTYPE_PAGE.read_bytes()


def test_home_quick_start_uses_the_store_prepare_flow():
    html = MAIN_PAGE.read_text(encoding="utf-8")

    assert 'id="quick-start-btn"' in html
    assert '一键启动' in html
    assert 'title="启动存衣准备：回到 A0 并伸出挂衣杆"' in html
    assert 'async function quickStartStore()' in html
    assert 'switchPage("store");' in html
    assert 'await newStore();' in html
    assert 'el("quick-start-btn").addEventListener("click", quickStartStore);' in html
    assert 'if (quickStartBtn) quickStartBtn.disabled = busy || step !== "ready";' in html


def test_fetch_completion_requires_manual_full_reset_confirmation():
    html = MAIN_PAGE.read_text(encoding="utf-8")

    assert 'id="fetch-reset-btn" hidden>衣物已取下，缩回并复位</button>' in html
    assert 'const STORE_RETRACT_ACTION_ID = 13;' in html
    assert 'const FETCH_RESET_ACTION_ID = 14;' in html
    assert 'async function resetAfterFetch()' in html
    assert 'sendTaskAction(FETCH_RESET_ACTION_ID, "取衣后机构复位", "fetch-reset")' in html
    assert 'await waitForTaskTerminal(commandId);' in html
    assert html.count('setFetchFlowStep("await-reset"') >= 3
    assert 'rememberFetchResetPending(true);' in html
    assert 'rememberFetchResetPending(false);' in html
    assert 'el("fetch-reset-btn").addEventListener("click", resetAfterFetch);' in html
    assert '"fetch-reset": "取衣复位"' in html


def test_reset_button_is_not_opened_when_fetch_fails():
    html = MAIN_PAGE.read_text(encoding="utf-8")

    assert "取衣动作未完成，机构复位按钮不会开放" in html
    assert "AI 取衣动作未完成，机构复位按钮不会开放" in html
    assert "机构复位失败或未收到完成回执" in html
    assert "请先取下衣物并完成机构复位，再开始存衣" in html
    assert "请先取下衣物并完成机构复位，再进行归还" in html


def test_pending_store_is_not_rendered_as_fetched():
    html = MAIN_PAGE.read_text(encoding="utf-8")

    assert 'function wardrobeStatusMeta(status)' in html
    assert '"pending-store": { text: "入库确认中", className: "status-tag pending" }' in html
    assert '"store-error": { text: "入库异常", className: "status-tag error" }' in html
    assert '"inventory-error": { text: "位置待核对", className: "status-tag error" }' in html
    assert "inventory_conflict: item.inventory_conflict || null" in html
    assert 'const statusMeta = wardrobeStatusMeta(item.status);' in html
    assert 'const tagText = selected ? "已选" : statusMeta.text;' in html
    assert 'const actionDisabled = inWardrobe || inUse ? "" : "disabled";' in html
    assert 'const tagText = selected ? "已选" : inWardrobe ? "在柜" : "已取出";' not in html


def test_cloud_task_and_serial_dispatch_is_atomic_with_rollback_feedback():
    html = MAIN_PAGE.read_text(encoding="utf-8")

    assert 'postJson("/api/hardware/task-dispatch"' in html
    assert 'task: { type, item_ids: itemIds, payload }' in html
    assert 'command,' in html
    assert '刚创建的云端任务已撤销' in html
    assert '云端任务已标记为下发失败' in html
    assert '任务已加入云端队列，但串口下发失败' not in html
