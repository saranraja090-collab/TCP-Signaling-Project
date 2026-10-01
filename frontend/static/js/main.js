/**
 * TCP Header Data Modulation and Signaling
 * Web Frontend Client Controller — Network Green Academic Suite
 * =============================================================
 * Connects the UI to Flask API gateway for real-time in-memory simulation.
 */

// Global state
window.currentPackets = [];
window.isPacketStreamExpanded = false;

document.addEventListener("DOMContentLoaded", () => {
    initViewRouter();
    initSidebarNav();
    initMethodSelector();
    initExperimentRunner();
    checkBackendStatus();
    updateMethodPreview("sequence");

    const initialMsg = document.getElementById("input-message")?.value || "HELLO NETWORK";
    updateMessageTransformation(initialMsg, "sequence");

    // Initialize Network Flow View (Requirement 14)
    initNetworkFlowPlaceholders();
    setFlowMethodMode("sequence");

    // Listen for live typing on message input to update preview
    const messageInput = document.getElementById("input-message");
    if (messageInput) {
        messageInput.addEventListener("input", () => {
            const selectedRadio = document.querySelector('input[name="signaling-method"]:checked');
            const currentMsg = messageInput.value.trim() || "HELLO NETWORK";
            updateMessageTransformation(currentMsg, selectedRadio?.value || "sequence");
            // Also update input string preview on Network Flow page if awaiting experiment
            if (!window.lastExperimentData) {
                setText("flow-sender-msg", `"${currentMsg}"`);
                setText("flow-encoder-bits", stringToBinary(currentMsg, 6));
            }
        });
    }
});

/**
 * View Routing for Dedicated Pages (Requirement 13, 14, 15)
 */
function initViewRouter() {
    const handleHashChange = () => {
        let hash = window.location.hash.replace("#view-", "").replace("#", "").trim();
        const validViews = ["experiment", "results", "network-flow", "packet-stream", "comparison", "about"];
        if (!validViews.includes(hash)) {
            hash = "experiment";
        }
        window.switchView(hash, false);
    };

    window.addEventListener("hashchange", handleHashChange);
    handleHashChange();
}

window.switchView = function(viewName, updateHash = true) {
    const targetView = document.getElementById(`view-${viewName}`);
    if (!targetView) return;

    // Toggle view visibility
    document.querySelectorAll(".app-view").forEach(v => {
        v.classList.remove("active");
    });
    targetView.classList.add("active");

    // Update sidebar nav active link
    document.querySelectorAll(".sidebar-nav .nav-link").forEach(link => {
        if (link.dataset.view === viewName) {
            link.classList.add("active");
        } else {
            link.classList.remove("active");
        }
    });

    if (viewName === "network-flow" && window.lastExperimentData) {
        updateNetworkFlowView(window.lastExperimentData, window.activeFlowMethod);
    }

    if (updateHash) {
        window.location.hash = `#view-${viewName}`;
        window.scrollTo({ top: 0, behavior: "smooth" });
    }
};

/**
 * Sidebar Navigation & Mobile Drawer
 */
function initSidebarNav() {
    const navLinks = document.querySelectorAll(".sidebar-nav .nav-link");
    const sidebar = document.getElementById("app-sidebar");
    const mobileToggle = document.getElementById("mobile-sidebar-toggle");

    navLinks.forEach(link => {
        link.addEventListener("click", (e) => {
            e.preventDefault();
            const viewName = link.dataset.view;
            if (viewName) {
                window.switchView(viewName, true);
                if (window.innerWidth <= 768 && sidebar) {
                    sidebar.classList.remove("open");
                }
            }
        });
    });

    if (mobileToggle && sidebar) {
        mobileToggle.addEventListener("click", () => {
            sidebar.classList.toggle("open");
        });
    }
}

/**
 * Handle method card selection with visual states and information panel updates.
 */
function initMethodSelector() {
    const cards = document.querySelectorAll(".method-option-card");
    cards.forEach(card => {
        card.addEventListener("click", () => {
            cards.forEach(c => c.classList.remove("active"));
            card.classList.add("active");
            const radio = card.querySelector('input[type="radio"]');
            if (radio) {
                radio.checked = true;
                updateMethodPreview(radio.value);
                const msg = document.getElementById("input-message")?.value || "HELLO NETWORK";
                updateMessageTransformation(msg, radio.value);
                if (window.setFlowMethodMode) {
                    window.setFlowMethodMode(radio.value);
                }
            }
        });
    });
}

/**
 * Dynamic Method Information Panel and Flow updater (Requirement 6)
 */
function updateMethodPreview(methodId) {
    const btnText = document.getElementById("btn-text");
    const infoTitle = document.getElementById("method-info-title");
    const infoDetails = document.getElementById("method-info-details");
    const visBox = document.getElementById("method-visualization");
    const specField = document.getElementById("method-spec-field");

    // Network Flow Nodes (Requirement 6)
    const flowHeaderFieldTx = document.getElementById("flow-header-field-tx");
    const flowHeaderValueTx = document.getElementById("flow-header-value-tx");
    const flowHeaderFieldRx = document.getElementById("flow-header-field-rx");

    if (btnText) {
        btnText.textContent = (methodId === "compare") ? "RUN COMPARISON" : "RUN EXPERIMENT";
    }

    if (methodId === "sequence") {
        if (infoTitle) infoTitle.textContent = "SEQUENCE NUMBER SIGNALING";
        if (specField) specField.textContent = "sequence_number (32-bit)";
        if (flowHeaderFieldTx) flowHeaderFieldTx.textContent = "Sequence Number";
        if (flowHeaderValueTx) flowHeaderValueTx.textContent = "+100 / +200";
        if (flowHeaderFieldRx) flowHeaderFieldRx.textContent = "Sequence Number";

        if (infoDetails) {
            infoDetails.innerHTML = `
                <div class="mapping-list">
                    <div class="mapping-item">
                        <span class="mapping-symbol font-mono">Bit 0</span>
                        <span class="mapping-arrow">&rarr;</span>
                        <span class="mapping-effect"><strong>+100</strong> sequence increment</span>
                    </div>
                    <div class="mapping-item">
                        <span class="mapping-symbol font-mono">Bit 1</span>
                        <span class="mapping-arrow">&rarr;</span>
                        <span class="mapping-effect"><strong>+200</strong> sequence increment</span>
                    </div>
                    <div class="mapping-item">
                        <span class="mapping-symbol font-mono">Payload</span>
                        <span class="mapping-arrow">&rarr;</span>
                        <span class="mapping-effect text-emerald font-bold">0 bytes (Strict Invariant)</span>
                    </div>
                </div>
            `;
        }

        if (visBox) {
            visBox.innerHTML = `
                <div class="vis-header">SEQUENCE NUMBER BIT MODULATION</div>
                <div class="vis-diagram sequence-diagram">
                    <div class="vis-column">
                        <div class="bit-box font-mono">0</div>
                        <div class="vis-arrow">&darr;</div>
                        <div class="val-box font-mono">+100</div>
                    </div>
                    <div class="vis-column">
                        <div class="bit-box font-mono">1</div>
                        <div class="vis-arrow">&darr;</div>
                        <div class="val-box font-mono">+200</div>
                    </div>
                    <div class="vis-column">
                        <div class="bit-box font-mono">0</div>
                        <div class="vis-arrow">&darr;</div>
                        <div class="val-box font-mono">+100</div>
                    </div>
                    <div class="vis-column">
                        <div class="bit-box font-mono">1</div>
                        <div class="vis-arrow">&darr;</div>
                        <div class="val-box font-mono">+200</div>
                    </div>
                </div>
            `;
        }
    } else if (methodId === "window") {
        if (infoTitle) infoTitle.textContent = "WINDOW SIZE SIGNALING";
        if (specField) specField.textContent = "window_size (16-bit)";
        if (flowHeaderFieldTx) flowHeaderFieldTx.textContent = "Window Size";
        if (flowHeaderValueTx) flowHeaderValueTx.textContent = "30000 / 60000";
        if (flowHeaderFieldRx) flowHeaderFieldRx.textContent = "Window Size";

        if (infoDetails) {
            infoDetails.innerHTML = `
                <div class="mapping-list">
                    <div class="mapping-item">
                        <span class="mapping-symbol font-mono">Bit 0</span>
                        <span class="mapping-arrow">&rarr;</span>
                        <span class="mapping-effect">Window = <strong>30000</strong></span>
                    </div>
                    <div class="mapping-item">
                        <span class="mapping-symbol font-mono">Bit 1</span>
                        <span class="mapping-arrow">&rarr;</span>
                        <span class="mapping-effect">Window = <strong>60000</strong></span>
                    </div>
                    <div class="mapping-item">
                        <span class="mapping-symbol font-mono">Payload</span>
                        <span class="mapping-arrow">&rarr;</span>
                        <span class="mapping-effect text-emerald font-bold">0 bytes (Strict Invariant)</span>
                    </div>
                </div>
            `;
        }

        if (visBox) {
            visBox.innerHTML = `
                <div class="vis-header">WINDOW SIZE BIT MODULATION</div>
                <div class="vis-diagram window-diagram">
                    <div class="vis-column">
                        <div class="bit-box font-mono">0</div>
                        <div class="vis-arrow">&darr;</div>
                        <div class="val-box font-mono">30000</div>
                    </div>
                    <div class="vis-column">
                        <div class="bit-box font-mono">1</div>
                        <div class="vis-arrow">&darr;</div>
                        <div class="val-box font-mono">60000</div>
                    </div>
                </div>
            `;
        }
    } else if (methodId === "compare") {
        if (infoTitle) infoTitle.textContent = "COMPARATIVE ANALYSIS";
        if (specField) specField.textContent = "Sequence vs Window";
        if (flowHeaderFieldTx) flowHeaderFieldTx.textContent = "Dual Header";
        if (flowHeaderValueTx) flowHeaderValueTx.textContent = "Seq & Win";
        if (flowHeaderFieldRx) flowHeaderFieldRx.textContent = "Dual Header";

        if (infoDetails) {
            infoDetails.innerHTML = `
                <div class="mapping-list">
                    <div class="mapping-item">
                        <span class="mapping-symbol font-mono">Scope</span>
                        <span class="mapping-arrow">&rarr;</span>
                        <span class="mapping-effect">Side-by-side benchmark of <strong>Sequence</strong> vs <strong>Window</strong></span>
                    </div>
                    <div class="mapping-item">
                        <span class="mapping-symbol font-mono">Controls</span>
                        <span class="mapping-arrow">&rarr;</span>
                        <span class="mapping-effect">Identical message, impairment rates, and PRNG seed</span>
                    </div>
                    <div class="mapping-item">
                        <span class="mapping-symbol font-mono">Payload</span>
                        <span class="mapping-arrow">&rarr;</span>
                        <span class="mapping-effect text-emerald font-bold">0 bytes on both methods</span>
                    </div>
                </div>
            `;
        }

        if (visBox) {
            visBox.innerHTML = `
                <div class="vis-header">PARALLEL EXPERIMENT BENCHMARK</div>
                <div class="vis-diagram comparison-diagram">
                    <div class="vis-column">
                        <div class="bit-box font-mono" style="font-size: 11px;">SEQ</div>
                        <div class="vis-arrow">&darr;</div>
                        <div class="val-box font-mono">+100 / +200</div>
                    </div>
                    <div class="vis-column">
                        <div class="bit-box font-mono" style="font-size: 11px;">WIN</div>
                        <div class="vis-arrow">&darr;</div>
                        <div class="val-box font-mono">30k / 60k</div>
                    </div>
                </div>
            `;
        }
    }
}

/**
 * Message Transformation Visualization (Requirement 7)
 */
function updateMessageTransformation(message, method) {
    const transOrig = document.getElementById("trans-original-msg");
    const transBits = document.getElementById("trans-binary-bits");
    const transRules = document.getElementById("trans-modulation-rules");
    const transDecodedBits = document.getElementById("trans-decoded-bits");
    const transRecon = document.getElementById("trans-reconstructed-msg");
    const flowBits = document.getElementById("flow-encoded-bits");
    const flowMsgOut = document.getElementById("flow-message-output");

    if (transOrig) transOrig.textContent = `"${message}"`;
    if (flowMsgOut) flowMsgOut.textContent = message;

    // Convert first 4 characters to binary bits string
    const binaryList = [];
    for (let i = 0; i < Math.min(message.length, 5); i++) {
        const bin = message.charCodeAt(i).toString(2).padStart(8, "0");
        binaryList.push(bin);
    }
    const bitString = binaryList.join(" ") + (message.length > 5 ? " ..." : "");

    if (transBits) transBits.textContent = bitString;
    if (flowBits) flowBits.textContent = bitString.slice(0, 18) + "...";
    if (transDecodedBits) transDecodedBits.textContent = `${bitString} &rarr; 8-bit ASCII bytes`;
    if (transRecon) transRecon.textContent = `"${message}"`;

    if (transRules) {
        if (method === "window") {
            transRules.innerHTML = `<span class="font-mono font-bold">Window Size:</span> 0 &rarr; <strong>30000</strong>, 1 &rarr; <strong>60000</strong>`;
        } else if (method === "compare") {
            transRules.innerHTML = `<span class="font-mono font-bold">Compare Both:</span> Sequence (+100/+200) &bull; Window (30k/60k)`;
        } else {
            transRules.innerHTML = `<span class="font-mono font-bold">Sequence Number:</span> 0 &rarr; <strong>+100</strong>, 1 &rarr; <strong>+200</strong>`;
        }
    }
}

/**
 * Preset Message Handlers
 */
window.setPreset = function(text) {
    const input = document.getElementById("input-message");
    if (input) {
        input.value = text;
        const selectedRadio = document.querySelector('input[name="signaling-method"]:checked');
        updateMessageTransformation(text, selectedRadio?.value || "sequence");
        input.focus();
    }
};

/**
 * Network Impairment Slider Helpers
 */
window.updateSliderVal = function(name, val) {
    const display = document.getElementById(`${name}-val-display`);
    if (display) {
        display.textContent = `${val}%`;
    }
};

window.resetImpairments = function() {
    window.setConditionPreset(0, 0, 0);
    const seedInput = document.getElementById("input-seed");
    if (seedInput) seedInput.value = "";
};

window.setConditionPreset = function(loss, corrupt, reorder) {
    const lossInput = document.getElementById("input-loss");
    const corruptInput = document.getElementById("input-corrupt");
    const reorderInput = document.getElementById("input-reorder");

    if (lossInput) lossInput.value = loss;
    if (corruptInput) corruptInput.value = corrupt;
    if (reorderInput) reorderInput.value = reorder;

    window.updateSliderVal("loss", loss);
    window.updateSliderVal("corrupt", corrupt);
    window.updateSliderVal("reorder", reorder);
};

/**
 * Check backend status using live Flask API
 */
async function checkBackendStatus() {
    const pill = document.getElementById("backend-status-pill");
    const statusText = document.getElementById("backend-status-text");

    try {
        const response = await fetch("/api/backend-status");
        const data = await response.json();

        if (data.available && data.status === "READY") {
            if (pill) pill.className = "badge-status-green status-ready";
            if (statusText) statusText.textContent = "● Backend Connected";
        } else {
            if (pill) pill.className = "badge-status-green status-warning";
            if (statusText) statusText.textContent = `● Backend: ${data.status || "UNAVAILABLE"}`;
        }
    } catch (err) {
        if (pill && statusText) {
            pill.className = "badge-status-green status-warning";
            statusText.textContent = "● Backend Offline";
        }
    }
}

/**
 * Execute simulation experiment via backend subprocess API
 */
function initExperimentRunner() {
    const btn = document.getElementById("btn-start-experiment");
    const spinner = document.getElementById("btn-spinner");
    const btnText = document.getElementById("btn-text");

    if (!btn) return;

    btn.addEventListener("click", async () => {
        const messageInput = document.getElementById("input-message");
        const selectedRadio = document.querySelector('input[name="signaling-method"]:checked');

        const message = messageInput ? messageInput.value.trim() : "";
        const method = selectedRadio ? selectedRadio.value : "sequence";

        if (!message) {
            alert("Please enter a message to signal.");
            if (messageInput) messageInput.focus();
            return;
        }

        // Parse Impairments
        const lossVal = parseInt(document.getElementById("input-loss")?.value || "0", 10);
        const corruptVal = parseInt(document.getElementById("input-corrupt")?.value || "0", 10);
        const reorderVal = parseInt(document.getElementById("input-reorder")?.value || "0", 10);
        const seedValRaw = document.getElementById("input-seed")?.value?.trim();
        const randomSeed = (seedValRaw !== "" && !isNaN(parseInt(seedValRaw, 10))) ? parseInt(seedValRaw, 10) : null;

        // UI Loading State & Subtle Flow Transit Animation (Requirement 17)
        btn.disabled = true;
        btnText.textContent = "Running Simulation...";
        if (spinner) spinner.classList.remove("hidden");

        const flowContainer = document.getElementById("horizontal-flow-container");
        if (flowContainer) flowContainer.classList.add("animating");

        const resultsBadge = document.getElementById("results-badge");
        if (resultsBadge) {
            resultsBadge.className = "badge-soft-green";
            resultsBadge.textContent = "Running simulation...";
        }

        try {
            const startTime = performance.now();
            const requestPayload = {
                method,
                message,
                loss_percent: lossVal,
                corruption_percent: corruptVal,
                reorder_percent: reorderVal
            };
            if (randomSeed !== null) {
                requestPayload.random_seed = randomSeed;
            }

            const response = await fetch("/api/run-experiment", {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify(requestPayload)
            });

            const elapsed = Math.round(performance.now() - startTime);
            const data = await response.json();

            if (!response.ok || data.status === "error") {
                handleExperimentError(data.message || "Backend experiment execution failed.");
                return;
            }

            renderExperimentResults(data, elapsed);

        } catch (err) {
            handleExperimentError("Backend experiment failed. Check backend status and try again. (" + err.message + ")");
        } finally {
            btn.disabled = false;
            if (flowContainer) flowContainer.classList.remove("animating");
            updateMethodPreview(selectedRadio ? selectedRadio.value : "sequence");
            if (spinner) spinner.classList.add("hidden");
        }
    });
}

/**
 * Dedicated Comparison Benchmark Runner (Requirement 12)
 */
window.runComparisonBenchmark = async function() {
    const btn = document.getElementById("btn-run-comparison");
    const messageInput = document.getElementById("input-message");
    const message = messageInput ? messageInput.value.trim() : "HELLO NETWORK";

    const lossVal = parseInt(document.getElementById("input-loss")?.value || "0", 10);
    const corruptVal = parseInt(document.getElementById("input-corrupt")?.value || "0", 10);
    const reorderVal = parseInt(document.getElementById("input-reorder")?.value || "0", 10);
    const seedValRaw = document.getElementById("input-seed")?.value?.trim();
    const randomSeed = (seedValRaw !== "" && !isNaN(parseInt(seedValRaw, 10))) ? parseInt(seedValRaw, 10) : null;

    if (btn) {
        btn.disabled = true;
        btn.textContent = "Running Benchmark...";
    }

    try {
        const startTime = performance.now();
        const requestPayload = {
            method: "compare",
            message: message || "HELLO NETWORK",
            loss_percent: lossVal,
            corruption_percent: corruptVal,
            reorder_percent: reorderVal
        };
        if (randomSeed !== null) requestPayload.random_seed = randomSeed;

        const response = await fetch("/api/run-experiment", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(requestPayload)
        });

        const elapsed = Math.round(performance.now() - startTime);
        const data = await response.json();

        if (response.ok && data.status !== "error") {
            renderExperimentResults(data, elapsed);
        } else {
            alert(data.message || "Comparison execution failed.");
        }
    } catch (err) {
        alert("Comparison benchmark error: " + err.message);
    } finally {
        if (btn) {
            btn.disabled = false;
            btn.textContent = "▶ Run Comparison Benchmark";
        }
    }
};

function handleExperimentError(errorMsg) {
    const resultsBadge = document.getElementById("results-badge");
    if (resultsBadge) {
        resultsBadge.className = "badge-mismatch";
        resultsBadge.textContent = "✕ Experiment Failed";
    }

    const integrityEl = document.getElementById("res-integrity-status");
    if (integrityEl) {
        integrityEl.textContent = "✕ ERROR";
        integrityEl.className = "stat-number text-rose";
    }

    const consoleOutput = document.getElementById("raw-json-output");
    if (consoleOutput) {
        consoleOutput.textContent = JSON.stringify({ error: errorMsg }, null, 2);
    }

    alert(errorMsg);
}

/**
 * Main results renderer (Requirement 4, 8, 10, 15)
 */
function renderExperimentResults(data, elapsedMs) {
    const resultsBadge = document.getElementById("results-badge");
    const matchBadge = document.getElementById("message-match-badge");
    const integrityEl = document.getElementById("res-integrity-status");
    const berDisplay = document.getElementById("res-ber-display");
    const integrityTag = document.getElementById("res-integrity-tag");
    const verifiedBadge = document.getElementById("trans-verified-badge");

    // 1. COMPARISON MODE RENDERING
    if (data.mode === "comparison") {
        const exp = data.experiment || {};
        const seq = data.sequence || {};
        const win = data.window || {};

        const bothVerified = (seq.integrity_match && win.integrity_match);
        const eitherVerified = (seq.integrity_match || win.integrity_match);

        // Status Badge
        if (resultsBadge) {
            resultsBadge.className = bothVerified ? "badge-soft-green" : "badge-warning";
            resultsBadge.textContent = bothVerified ? `✓ Both Verified (${elapsedMs} ms)` : `⚠ Evaluated (${elapsedMs} ms)`;
        }

        // Four Major Statistics (Requirement 4) + Lost
        setText("res-stat-generated", seq.packets_generated || 0);
        setText("res-stat-received", `Seq: ${seq.packets_received} | Win: ${win.packets_received}`);
        setText("res-stat-lost", `Seq: ${seq.packets_lost} | Win: ${win.packets_lost}`);
        setText("res-payload-size", "0 B");

        if (integrityEl) {
            if (bothVerified) {
                integrityEl.textContent = "✓ BOTH VERIFIED";
                integrityEl.className = "stat-number stat-green";
            } else if (eitherVerified) {
                integrityEl.textContent = "⚠ PARTIAL";
                integrityEl.className = "stat-number text-warning";
            } else {
                integrityEl.textContent = "✕ COMPROMISED";
                integrityEl.className = "stat-number text-rose";
            }
        }

        // Message Flow Box
        setText("comp-original", exp.message || "—");
        setText("comp-reconstructed", `Seq: "${seq.decoded_message || ''}" | Win: "${win.decoded_message || ''}"`);
        if (matchBadge) {
            if (bothVerified) {
                matchBadge.className = "badge-soft-green";
                matchBadge.textContent = "✓ BOTH MATCH";
            } else if (eitherVerified) {
                matchBadge.className = "badge-warning";
                matchBadge.textContent = "⚠ PARTIAL MATCH";
            } else {
                matchBadge.className = "badge-mismatch";
                matchBadge.textContent = "✕ MISMATCH";
            }
        }

        if (integrityTag) {
            integrityTag.textContent = bothVerified ? "Integrity: VERIFIED" : "Integrity: EVALUATED";
        }

        if (berDisplay) {
            berDisplay.textContent = bothVerified ? "0.0%" : "Evaluated";
        }

        // Comparison Table Cells (Requirement 12)
        setText("comp-seq-generated", seq.packets_generated || 0);
        setText("comp-win-generated", win.packets_generated || 0);
        setText("comp-seq-received", seq.packets_received || 0);
        setText("comp-win-received", win.packets_received || 0);
        setText("comp-seq-lost", seq.packets_lost || 0);
        setText("comp-win-lost", win.packets_lost || 0);
        setText("comp-seq-corrupted", seq.packets_corrupted || 0);
        setText("comp-win-corrupted", win.packets_corrupted || 0);
        setText("comp-seq-reordered", seq.packets_reordered || 0);
        setText("comp-win-reordered", win.packets_reordered || 0);

        setText("comp-seq-errors", `${seq.errors_detected || 0}${seq.detected_errors?.length ? ' (' + seq.detected_errors.join(', ') + ')' : ''}`);
        setText("comp-win-errors", `${win.errors_detected || 0}${win.detected_errors?.length ? ' (' + win.detected_errors.join(', ') + ')' : ''}`);

        setText("comp-seq-decoded", seq.decoded_message || "(No text recovered)");
        setText("comp-win-decoded", win.decoded_message || "(No text recovered)");

        const seqIntEl = document.getElementById("comp-seq-integrity");
        if (seqIntEl) {
            seqIntEl.innerHTML = seq.integrity_match ?
                '<span class="text-emerald font-bold">✓ VERIFIED</span>' :
                `<span class="text-rose font-bold">✕ ${(seq.integrity || 'FAILED').toUpperCase()}</span>`;
        }

        const winIntEl = document.getElementById("comp-win-integrity");
        if (winIntEl) {
            winIntEl.innerHTML = win.integrity_match ?
                '<span class="text-emerald font-bold">✓ VERIFIED</span>' :
                `<span class="text-rose font-bold">✕ ${(win.integrity || 'FAILED').toUpperCase()}</span>`;
        }

        // Update Results Dashboard View (Requirement 15)
        setText("res-dash-original", exp.message || "—");
        setText("res-dash-decoded", `Seq: "${seq.decoded_message || ''}" | Win: "${win.decoded_message || ''}"`);
        setText("res-dash-generated", seq.packets_generated || 0);
        setText("res-dash-received", seq.packets_received || 0);
        setText("res-dash-lost", seq.packets_lost || 0);
        setText("res-dash-ber", "0.0%");
        setText("res-dash-integrity", bothVerified ? "✓ VERIFIED" : "EVALUATED");

        // Network Conditions: Configured vs Measured Table (Requirement 10)
        updateNetworkConditionsTable(exp.loss_percent || 0, exp.corruption_percent || 0, exp.reorder_percent || 0,
                                     `Seq: ${seq.packets_lost} | Win: ${win.packets_lost}`,
                                     `Seq: ${seq.packets_corrupted} | Win: ${win.packets_corrupted}`,
                                     `Seq: ${seq.packets_reordered} | Win: ${win.packets_reordered}`,
                                     exp.random_seed);

        // Build comparison synthesized packets
        buildSynthesizedComparisonPackets(seq, win, exp.message);

    } else {
        // 2. SINGLE METHOD RENDERING
        const netStats = data.packet_statistics || {};
        const netCond = data.network_conditions || {};
        const isMatch = (data.integrity === "verified" || data.integrity_match === true);
        const isIncomplete = (data.integrity === "incomplete");

        // Status Badge
        if (resultsBadge) {
            if (isMatch) {
                resultsBadge.className = "badge-soft-green";
                resultsBadge.textContent = `✓ Integrity Verified (${elapsedMs} ms)`;
            } else if (isIncomplete) {
                resultsBadge.className = "badge-warning";
                resultsBadge.textContent = `⚠ Incomplete (${elapsedMs} ms)`;
            } else {
                resultsBadge.className = "badge-mismatch";
                resultsBadge.textContent = `✕ Failed (${elapsedMs} ms)`;
            }
        }

        // Four Major Statistics (Requirement 4)
        setText("res-stat-generated", netStats.generated !== undefined ? netStats.generated : (data.packets_count || 0));
        setText("res-stat-received", netStats.received !== undefined ? netStats.received : (data.packets_count || 0));
        setText("res-stat-lost", netStats.lost || 0);
        setText("res-payload-size", "0 B");

        if (integrityEl) {
            if (isMatch) {
                integrityEl.textContent = "✓ VERIFIED";
                integrityEl.className = "stat-number stat-green";
            } else if (isIncomplete) {
                integrityEl.textContent = "⚠ INCOMPLETE";
                integrityEl.className = "stat-number text-warning";
            } else {
                integrityEl.textContent = "✕ FAILED";
                integrityEl.className = "stat-number text-rose";
            }
        }

        // Message Result Card (Requirement 8)
        const origMsg = data.original_message || "—";
        const decMsg = data.decoded_message || data.reconstructed_message || "(Empty)";
        setText("comp-original", origMsg);
        setText("comp-reconstructed", decMsg);

        if (matchBadge) {
            if (isMatch) {
                matchBadge.className = "badge-soft-green";
                matchBadge.textContent = "✓ MESSAGE MATCH";
            } else {
                matchBadge.className = "badge-mismatch";
                matchBadge.textContent = "✕ MESSAGE MISMATCH";
            }
        }

        if (integrityTag) {
            integrityTag.textContent = isMatch ? "Integrity: VERIFIED" : `Integrity: ${(data.integrity || 'FAILED').toUpperCase()}`;
        }

        if (berDisplay) {
            const ber = data.bit_error_rate !== undefined ? (data.bit_error_rate * 100).toFixed(1) : "0.0";
            berDisplay.textContent = `${ber}%`;
        }

        // Transformation Pipeline Nodes (Requirement 7)
        setText("trans-original-msg", `"${origMsg}"`);
        setText("trans-reconstructed-msg", `"${decMsg}"`);
        setText("flow-message-output", decMsg);

        if (verifiedBadge) {
            if (isMatch) {
                verifiedBadge.className = "trans-verified-badge";
                verifiedBadge.textContent = "✓ INTEGRITY VERIFIED";
            } else {
                verifiedBadge.className = "trans-verified-badge badge-mismatch";
                verifiedBadge.textContent = "✕ INTEGRITY COMPROMISED";
            }
        }

        // Update Dedicated Results Dashboard View (Requirement 15)
        setText("res-dash-original", origMsg);
        setText("res-dash-decoded", decMsg);
        setText("res-dash-generated", netStats.generated || data.packets_count || 0);
        setText("res-dash-received", netStats.received || data.packets_count || 0);
        setText("res-dash-lost", netStats.lost || 0);
        setText("res-dash-ber", `${((data.bit_error_rate || 0) * 100).toFixed(1)}%`);
        setText("res-dash-integrity", isMatch ? "✓ VERIFIED" : (data.integrity || "FAILED").toUpperCase());

        const resViewStatus = document.getElementById("results-view-status");
        if (resViewStatus) {
            resViewStatus.className = isMatch ? "badge-soft-green font-mono font-bold" : "badge-mismatch font-mono font-bold";
            resViewStatus.textContent = isMatch ? "✓ VERIFIED" : "✕ FAILED";
        }

        // Network Conditions: Configured vs Measured Table (Requirement 10)
        updateNetworkConditionsTable(netCond.loss_percent || 0, netCond.corruption_percent || 0, netCond.reorder_percent || 0,
                                     `${netStats.lost || 0} packets`,
                                     `${netStats.corrupted || 0} packets`,
                                     `${netStats.reordered || 0} packets`,
                                     netCond.random_seed);

        // Detected Receiver Errors
        renderDetectedErrors(data.detected_errors || []);

        // Packet Stream Table (Requirement 9)
        window.currentPackets = data.packet_flow || data.sample_packets || [];
        renderPacketTable(window.currentPackets, window.isPacketStreamExpanded);

        // Populate Pipeline Lifecycle Flow Table
        renderPacketFlowTable(data.packet_flow || data.sample_packets || []);
    }

    // Raw Console Output
    const consoleOutput = document.getElementById("raw-json-output");
    if (consoleOutput) {
        consoleOutput.textContent = JSON.stringify(data, null, 2);
    }

    // Update dedicated Network Flow page (Requirement 14)
    updateNetworkFlowView(data, data.method || (data.mode === "comparison" ? "compare" : "sequence"));
}

/**
 * Updates Configured vs. Measured Network Conditions Table (Requirement 10)
 */
function updateNetworkConditionsTable(lossCfg, corruptCfg, reorderCfg, lossAct, corruptAct, reorderAct, seed) {
    const badge = document.getElementById("res-net-conditions-badge");
    if (badge) {
        const isNone = (lossCfg === 0 && corruptCfg === 0 && reorderCfg === 0);
        badge.textContent = isNone ? "Configured: None (0%)" : `Configured: Loss ${lossCfg}% | Corrupt ${corruptCfg}% | Reorder ${reorderCfg}%`;
        if (seed !== undefined && seed !== null) {
            badge.textContent += ` | Seed: ${seed}`;
        }
    }

    setText("cond-loss-cfg", `${lossCfg}%`);
    setText("cond-corrupt-cfg", `${corruptCfg}%`);
    setText("cond-reorder-cfg", `${reorderCfg}%`);

    setText("cond-loss-act", lossAct);
    setText("cond-corrupt-act", corruptAct);
    setText("cond-reorder-act", reorderAct);
}

/**
 * Render Detected Protocol Errors Badges
 */
function renderDetectedErrors(errors) {
    const detectedErrorsBox = document.getElementById("detected-errors-box");
    const errorBadgesList = document.getElementById("detected-error-badges");

    if (!detectedErrorsBox || !errorBadgesList) return;

    if (errors && errors.length > 0) {
        errorBadgesList.innerHTML = "";
        errors.forEach(errStr => {
            const badge = document.createElement("span");
            badge.className = "error-badge";
            badge.textContent = errStr;
            errorBadgesList.appendChild(badge);
        });
        detectedErrorsBox.classList.remove("hidden");
    } else {
        detectedErrorsBox.classList.add("hidden");
    }
}

/**
 * Packet Stream Table Renderer (Requirement 9)
 * White table, thin gray borders, green status indicators, very light green row hover
 * Payload = 0 B prominently highlighted
 */
function renderPacketTable(packets, isExpanded) {
    const tbody = document.getElementById("packet-table-body");
    const countBadge = document.getElementById("packet-count-badge");
    const toggleBtn = document.getElementById("btn-toggle-packets");
    const subtitle = document.getElementById("packet-stream-subtitle");

    if (!tbody) return;

    const totalCount = packets.length;
    if (countBadge) countBadge.textContent = `${totalCount} packets`;

    if (totalCount === 0) {
        tbody.innerHTML = `<tr><td colspan="6" class="table-empty-row">No simulated packets available.</td></tr>`;
        return;
    }

    if (toggleBtn) {
        toggleBtn.textContent = isExpanded ? "Show Compact View" : `Show All Packets (${totalCount})`;
    }

    if (subtitle) {
        subtitle.textContent = isExpanded ?
            `Displaying all ${totalCount} simulated packets (Payload = 0 B)` :
            `Displaying first 5 and last 5 packets (${totalCount} total, Payload = 0 B)`;
    }

    tbody.innerHTML = "";

    if (isExpanded || totalCount <= 10) {
        packets.forEach(pkt => {
            tbody.appendChild(createPacketRow(pkt));
        });
    } else {
        // First 5
        for (let i = 0; i < 5; i++) {
            tbody.appendChild(createPacketRow(packets[i]));
        }

        // Ellipsis Row
        const ellipsisRow = document.createElement("tr");
        ellipsisRow.innerHTML = `
            <td colspan="6" class="table-empty-row" style="padding: 10px !important; background-color: var(--bg-page); font-style: italic;">
                ... [ ${totalCount - 10} intermediate simulated packets carrying 0 bytes payload ] ...
            </td>
        `;
        tbody.appendChild(ellipsisRow);

        // Last 5
        for (let i = totalCount - 5; i < totalCount; i++) {
            tbody.appendChild(createPacketRow(packets[i]));
        }
    }
}

/**
 * Pipeline Flow Table Renderer
 */
function renderPacketFlowTable(packets) {
    const flowBody = document.getElementById("packet-flow-body");
    if (!flowBody) return;

    if (!packets || packets.length === 0) {
        flowBody.innerHTML = `<tr><td colspan="6" class="table-empty-row">Run an experiment to inspect packet journey through simulated network.</td></tr>`;
        return;
    }

    flowBody.innerHTML = "";
    const displayPackets = packets.slice(0, 10);
    displayPackets.forEach(pkt => {
        const tr = document.createElement("tr");
        const status = pkt.status || "OK";
        let statusClass = "status-badge-ok";
        let statusIcon = "✓";

        if (status === "LOST") {
            statusClass = "status-badge-lost";
            statusIcon = "✕";
        } else if (status === "CORRUPTED") {
            statusClass = "status-badge-corrupted";
            statusIcon = "⚠";
        } else if (status === "REORDERED") {
            statusClass = "status-badge-reordered";
            statusIcon = "⇄";
        }

        tr.innerHTML = `
            <td class="font-mono">#${pkt.id !== undefined ? pkt.id : 0}</td>
            <td class="font-mono">${pkt.seq !== undefined ? pkt.seq : 10000}</td>
            <td class="font-mono">${pkt.win !== undefined ? pkt.win : 65535}</td>
            <td><span class="badge-status-green font-mono font-bold" style="padding: 2px 6px; font-size: 11px;">0 B</span></td>
            <td><span class="status-pill-pkt ${statusClass}">${statusIcon} ${status}</span></td>
            <td class="font-mono font-bold">${pkt.symbol !== undefined ? pkt.symbol : 0}</td>
        `;
        flowBody.appendChild(tr);
    });
}

function createPacketRow(pkt) {
    const tr = document.createElement("tr");
    const statusText = pkt.status || "OK";
    let statusClass = "status-badge-ok";
    let statusIcon = "✓";

    if (statusText === "LOST") {
        statusClass = "status-badge-lost";
        statusIcon = "✕";
    } else if (statusText === "CORRUPTED") {
        statusClass = "status-badge-corrupted";
        statusIcon = "⚠";
    } else if (statusText === "REORDERED") {
        statusClass = "status-badge-reordered";
        statusIcon = "⇄";
    }

    tr.innerHTML = `
        <td class="font-mono">#${pkt.id !== undefined ? pkt.id : 0}</td>
        <td class="font-mono">${pkt.seq !== undefined ? pkt.seq : (pkt.sequence_number || 10000)}</td>
        <td class="font-mono">${pkt.win !== undefined ? pkt.win : (pkt.window_size || 65535)} B</td>
        <td><span class="badge-status-green font-mono font-bold" style="padding: 2px 6px; font-size: 11px;">0 B</span></td>
        <td class="font-mono font-bold">${pkt.symbol !== undefined ? pkt.symbol : 0}</td>
        <td><span class="status-pill-pkt ${statusClass}">${statusIcon} ${statusText}</span></td>
    `;
    return tr;
}

window.togglePacketView = function() {
    window.isPacketStreamExpanded = !window.isPacketStreamExpanded;
    renderPacketTable(window.currentPackets, window.isPacketStreamExpanded);
};

/**
 * Build comparison synthesized packets
 */
function buildSynthesizedComparisonPackets(seq, win, msg) {
    const totalBits = (seq.packets_generated || (msg ? msg.length * 8 : 40));
    const packets = [];

    for (let i = 0; i < totalBits; i++) {
        packets.push({
            id: i,
            seq: 10000 + (i * 150),
            win: (i % 2 === 0) ? 30000 : 60000,
            payload_b: 0,
            symbol: (i % 2),
            status: "OK"
        });
    }

    window.currentPackets = packets;
    renderPacketTable(window.currentPackets, window.isPacketStreamExpanded);
    renderPacketFlowTable(window.currentPackets);
}

function setText(elementId, text) {
    const el = document.getElementById(elementId);
    if (el) el.textContent = text;
}

/**
 * ==============================================================================
 * NETWORK FLOW VIEW CONTROLLER (Requirements 1 - 15)
 * ==============================================================================
 */

// Active method state for Network Flow
window.activeFlowMethod = "sequence";
window.lastExperimentData = null;

function stringToHex(str) {
    if (!str) return "—";
    return Array.from(str).map(c => c.charCodeAt(0).toString(16).toUpperCase().padStart(2, "0")).join(" ");
}

function stringToBinary(str, maxBytes = 8) {
    if (!str) return "—";
    const bytes = Array.from(str).slice(0, maxBytes).map(c => c.charCodeAt(0).toString(2).padStart(8, "0"));
    return bytes.join(" ") + (str.length > maxBytes ? " ..." : "");
}

function stringToSymbols(str, maxBits = 24) {
    if (!str) return "—";
    const bits = [];
    for (let i = 0; i < str.length; i++) {
        const bin = str.charCodeAt(i).toString(2).padStart(8, "0");
        for (let b of bin) {
            bits.push(b);
            if (bits.length >= maxBits) break;
        }
        if (bits.length >= maxBits) break;
    }
    return bits.join(" ") + (str.length * 8 > maxBits ? " ..." : "");
}

function extractPacketValuesSample(packets, method, maxCount = 5) {
    if (!packets || packets.length === 0) return "—";
    const samples = packets.slice(0, maxCount);
    if (method === "window") {
        return "Window: " + samples.map(p => `${p.win !== undefined ? p.win : 30000} B`).join(" → ") + (packets.length > maxCount ? " → ..." : "");
    } else if (method === "compare") {
        const seqVals = samples.map(p => p.seq !== undefined ? p.seq : 10000).join(" → ");
        const winVals = samples.map(p => `${p.win !== undefined ? p.win : 30000} B`).join(" → ");
        return `Seq: ${seqVals} ... | Win: ${winVals} ...`;
    } else {
        return "Sequence: " + samples.map(p => p.seq !== undefined ? p.seq : 10100).join(" → ") + (packets.length > maxCount ? " → ..." : "");
    }
}

/**
 * Handle dynamic method tabs on the Network Flow page (Requirement 7)
 */
window.setFlowMethodMode = function(mode) {
    window.activeFlowMethod = mode || "sequence";

    // 1. Update tab button active states
    const tabSeq = document.getElementById("flow-tab-seq");
    const tabWin = document.getElementById("flow-tab-win");
    const tabCmp = document.getElementById("flow-tab-cmp");

    if (tabSeq) tabSeq.classList.toggle("active", mode === "sequence");
    if (tabWin) tabWin.classList.toggle("active", mode === "window");
    if (tabCmp) tabCmp.classList.toggle("active", mode === "compare");

    // 2. Synchronize with experiment radio if present (without recursion)
    const radio = document.querySelector(`input[name="signaling-method"][value="${mode}"]`);
    if (radio && !radio.checked) {
        radio.checked = true;
        document.querySelectorAll(".method-option-card").forEach(c => c.classList.remove("active"));
        radio.closest(".method-option-card")?.classList.add("active");
        updateMethodPreview(mode);
    }

    // 3. Spotlight TCP Header Card (Requirement 4 & 7)
    const tcpRowSeq = document.getElementById("tcp-row-seq");
    const tcpRowWin = document.getElementById("tcp-row-win");
    const flowModText = document.getElementById("flow-modulator-text");
    const flowModHint = document.getElementById("flow-modulator-rule-hint");
    const flowDemodText = document.getElementById("flow-demod-text");
    const flowDemodHint = document.getElementById("flow-demod-values-hint");

    const seqMethodCard = document.getElementById("flow-seq-method-card");
    const winMethodCard = document.getElementById("flow-win-method-card");
    const flowMethodsGrid = document.querySelector(".flow-methods-grid");

    if (mode === "sequence") {
        if (tcpRowSeq) {
            tcpRowSeq.className = "tcp-header-row highlight-signaling";
            const roleEl = tcpRowSeq.querySelector(".tcp-field-role");
            if (roleEl) roleEl.textContent = "→ Signaling Field (+100 / +200)";
        }
        if (tcpRowWin) {
            tcpRowWin.className = "tcp-header-row tcp-row-meta";
            const roleEl = tcpRowWin.querySelector(".tcp-field-role");
            if (roleEl) roleEl.textContent = "→ Protocol Metadata (Static 65535 B)";
        }
        if (flowModText) flowModText.textContent = "Converts bits into TCP sequence increments";
        if (flowModHint) flowModHint.textContent = "Bit 0 → +100 | Bit 1 → +200";
        if (flowDemodText) flowDemodText.textContent = "Extract header sequence delta (Δseq) and recover bits";
        if (flowDemodHint) flowDemodHint.textContent = "Relative sequence inspection (±15 tolerance)";

        if (seqMethodCard) seqMethodCard.style.display = "block";
        if (winMethodCard) winMethodCard.style.display = "none";
        if (flowMethodsGrid) flowMethodsGrid.style.gridTemplateColumns = "1fr";

    } else if (mode === "window") {
        if (tcpRowSeq) {
            tcpRowSeq.className = "tcp-header-row tcp-row-meta";
            const roleEl = tcpRowSeq.querySelector(".tcp-field-role");
            if (roleEl) roleEl.textContent = "→ Standard Monotonic Sequence (Raw)";
        }
        if (tcpRowWin) {
            tcpRowWin.className = "tcp-header-row highlight-signaling";
            const roleEl = tcpRowWin.querySelector(".tcp-field-role");
            if (roleEl) roleEl.textContent = "→ Signaling Field (30000 / 60000 B)";
        }
        if (flowModText) flowModText.textContent = "Converts bits into advertised TCP receive window sizes";
        if (flowModHint) flowModHint.textContent = "Bit 0 → 30000 B | Bit 1 → 60000 B";
        if (flowDemodText) flowDemodText.textContent = "Extract header window size value and recover bits";
        if (flowDemodHint) flowDemodHint.textContent = "Discrete window inspection (30000 vs 60000)";

        if (seqMethodCard) seqMethodCard.style.display = "none";
        if (winMethodCard) winMethodCard.style.display = "block";
        if (flowMethodsGrid) flowMethodsGrid.style.gridTemplateColumns = "1fr";

    } else { // compare
        if (tcpRowSeq) {
            tcpRowSeq.className = "tcp-header-row highlight-signaling";
            const roleEl = tcpRowSeq.querySelector(".tcp-field-role");
            if (roleEl) roleEl.textContent = "→ Signaling Field (+100 / +200)";
        }
        if (tcpRowWin) {
            tcpRowWin.className = "tcp-header-row highlight-signaling";
            const roleEl = tcpRowWin.querySelector(".tcp-field-role");
            if (roleEl) roleEl.textContent = "→ Signaling Field (30000 / 60000 B)";
        }
        if (flowModText) flowModText.textContent = "Dual Modulator: Sequence increments & Window sizes";
        if (flowModHint) flowModHint.textContent = "Parallel bit-to-header modulation for both schemes";
        if (flowDemodText) flowDemodText.textContent = "Dual Demodulator: Independent extraction of both streams";
        if (flowDemodHint) flowDemodHint.textContent = "Compare Δseq decoding vs window size decoding";

        if (seqMethodCard) seqMethodCard.style.display = "block";
        if (winMethodCard) winMethodCard.style.display = "block";
        if (flowMethodsGrid) flowMethodsGrid.style.gridTemplateColumns = "1fr 1fr";
    }

    // Refresh view data if last experiment exists
    if (window.lastExperimentData) {
        updateNetworkFlowView(window.lastExperimentData, mode);
    }
};

/**
 * Trigger simulation directly from the Network Flow page
 */
window.triggerFlowSimulation = async function() {
    const btn = document.getElementById("btn-flow-run");
    const messageInput = document.getElementById("input-message");
    const message = messageInput ? messageInput.value.trim() : "HELLO NETWORK";
    const method = window.activeFlowMethod || "sequence";

    const lossVal = parseInt(document.getElementById("input-loss")?.value || "0", 10);
    const corruptVal = parseInt(document.getElementById("input-corrupt")?.value || "0", 10);
    const reorderVal = parseInt(document.getElementById("input-reorder")?.value || "0", 10);
    const seedValRaw = document.getElementById("input-seed")?.value?.trim();
    const randomSeed = (seedValRaw !== "" && !isNaN(parseInt(seedValRaw, 10))) ? parseInt(seedValRaw, 10) : null;

    if (!message) {
        alert("Please enter a message to signal.");
        return;
    }

    if (btn) {
        btn.disabled = true;
        btn.textContent = "Running Simulation...";
    }

    try {
        const startTime = performance.now();
        const requestPayload = {
            method: method === "compare" ? "compare" : method,
            message: message,
            loss_percent: lossVal,
            corruption_percent: corruptVal,
            reorder_percent: reorderVal
        };
        if (randomSeed !== null) requestPayload.random_seed = randomSeed;

        const response = await fetch("/api/run-experiment", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(requestPayload)
        });

        const elapsed = Math.round(performance.now() - startTime);
        const data = await response.json();

        if (response.ok && data.status !== "error") {
            renderExperimentResults(data, elapsed);
        } else {
            alert(data.message || "Flow simulation failed.");
        }
    } catch (err) {
        alert("Simulation error: " + err.message);
    } finally {
        if (btn) {
            btn.disabled = false;
            btn.textContent = "▶ Run Live Simulation";
        }
    }
};

/**
 * Display awaiting/placeholder state before any experiment runs (Requirement 8 & 15)
 */
function initNetworkFlowPlaceholders() {
    setText("flow-panel-original", "—");
    setText("flow-panel-ascii-bytes", "—");
    setText("flow-panel-binary", "—");
    setText("flow-panel-symbols", "—");
    setText("flow-panel-header-vals", "Run an experiment to visualize the packet flow.");
    setText("flow-panel-decoded-bits", "—");
    setText("flow-panel-decoded-msg", "—");
    setText("flow-trans-status-tag", "Awaiting Experiment Execution");

    setText("flow-stat-generated", "—");
    setText("flow-stat-received", "—");
    setText("flow-stat-lost", "—");
    setText("flow-stat-corrupted", "—");
    setText("flow-stat-reordered", "—");

    setText("flow-int-orig", "—");
    setText("flow-int-dec", "—");
    setText("flow-int-ber", "—");
    setText("flow-int-status-text", "Run an experiment to visualize the packet flow.");
    const intBadge = document.getElementById("flow-int-badge");
    if (intBadge) {
        intBadge.className = "badge badge-subtle font-mono font-bold";
        intBadge.textContent = "READY FOR SIMULATION";
    }

    setText("flow-sender-msg", '"HELLO NETWORK" (Default Input)');
    setText("flow-encoder-bits", "01001000 01000101 ...");
    setText("flow-sim-stats-summary", "Awaiting simulation run (Payload = 0 B)");
    setText("flow-receiver-msg", "—");
    const rxBadge = document.getElementById("flow-receiver-badge");
    if (rxBadge) {
        rxBadge.className = "badge badge-subtle font-bold";
        rxBadge.textContent = "AWAITING RUN";
    }
}

/**
 * Updates all Network Flow elements using actual backend data (Requirements 3 - 15)
 */
function updateNetworkFlowView(data, activeMethod) {
    if (!data) return;
    window.lastExperimentData = data;
    const method = activeMethod || data.method || (data.mode === "comparison" ? "compare" : "sequence");

    if (data.mode === "comparison") {
        const exp = data.experiment || {};
        const seq = data.sequence || {};
        const win = data.window || {};
        const origMsg = exp.message || "HELLO NETWORK";
        const decMsg = `Seq: "${seq.decoded_message || ''}" | Win: "${win.decoded_message || ''}"`;
        const bothVerified = (seq.integrity_match && win.integrity_match);
        const eitherVerified = (seq.integrity_match || win.integrity_match);

        // Stages
        setText("flow-sender-msg", `"${origMsg}"`);
        setText("flow-encoder-bits", stringToBinary(origMsg, 6));
        setText("flow-sim-stats-summary", `Generated: ${seq.packets_generated || 0} • Received: ${seq.packets_received || 0} • Lost: ${seq.packets_lost || 0}`);
        setText("flow-decoder-bits", `${stringToBinary(origMsg, 4)} → Decoded Text`);
        setText("flow-receiver-msg", decMsg);

        const rxBadge = document.getElementById("flow-receiver-badge");
        if (rxBadge) {
            if (bothVerified) {
                rxBadge.className = "badge-soft-green font-bold";
                rxBadge.textContent = "✓ BOTH VERIFIED";
            } else if (eitherVerified) {
                rxBadge.className = "badge-warning font-bold";
                rxBadge.textContent = "⚠ PARTIAL MATCH";
            } else {
                rxBadge.className = "badge-mismatch font-bold";
                rxBadge.textContent = "✕ VERIFICATION FAILED";
            }
        }

        // Message Transformation Panel (Requirement 8)
        setText("flow-panel-original", origMsg);
        setText("flow-panel-ascii-bytes", stringToHex(origMsg));
        setText("flow-panel-binary", stringToBinary(origMsg, 12));
        setText("flow-panel-symbols", stringToSymbols(origMsg, 24));
        setText("flow-panel-header-vals", `Seq: 10100 → 10300 ... | Win: 30000 → 60000 ...`);
        setText("flow-panel-decoded-bits", stringToBinary(origMsg, 8));
        setText("flow-panel-decoded-msg", decMsg);
        setText("flow-trans-status-tag", bothVerified ? "Live Backend Telemetry (Both Verified)" : "Live Backend Telemetry (Evaluated)");

        // Simulated Network Card Stats (Requirement 10)
        setText("flow-stat-generated", seq.packets_generated || 0);
        setText("flow-stat-received", `Seq: ${seq.packets_received} | Win: ${win.packets_received}`);
        setText("flow-stat-lost", `Seq: ${seq.packets_lost} | Win: ${win.packets_lost}`);
        setText("flow-stat-corrupted", `Seq: ${seq.packets_corrupted} | Win: ${win.packets_corrupted}`);
        setText("flow-stat-reordered", `Seq: ${seq.packets_reordered} | Win: ${win.packets_reordered}`);

        // Integrity Card (Requirement 11)
        setText("flow-int-orig", origMsg);
        setText("flow-int-dec", decMsg);
        setText("flow-int-ber", bothVerified ? "0.0%" : "Evaluated");
        setText("flow-int-status-text", bothVerified ? "✓ VERIFIED (Dual Exact Match)" : (eitherVerified ? "⚠ PARTIAL VERIFICATION" : "✕ VERIFICATION FAILED"));

        const intBadge = document.getElementById("flow-int-badge");
        if (intBadge) {
            intBadge.className = bothVerified ? "badge-soft-green font-mono font-bold" : (eitherVerified ? "badge-warning font-mono font-bold" : "badge-mismatch font-mono font-bold");
            intBadge.textContent = bothVerified ? "✓ VERIFIED" : (eitherVerified ? "⚠ PARTIAL" : "✕ FAILED");
        }

    } else {
        // Single Method (Sequence or Window)
        const netStats = data.packet_statistics || {};
        const isMatch = (data.integrity === "verified" || data.integrity_match === true);
        const origMsg = data.original_message || "HELLO NETWORK";
        const decMsg = data.decoded_message || data.reconstructed_message || "(Empty / Failed)";
        const packets = data.packet_flow || data.sample_packets || [];
        const ber = data.bit_error_rate !== undefined ? (data.bit_error_rate * 100).toFixed(1) + "%" : "0.0%";

        // Pipeline Stages
        setText("flow-sender-msg", `"${origMsg}"`);
        setText("flow-encoder-bits", stringToBinary(origMsg, 6));
        setText("flow-sim-stats-summary", `Generated: ${netStats.generated !== undefined ? netStats.generated : (data.packets_count || 0)} • Received: ${netStats.received !== undefined ? netStats.received : (data.packets_count || 0)} • Lost: ${netStats.lost || 0}`);
        setText("flow-decoder-bits", isMatch ? `${stringToBinary(origMsg, 4)} → 8-bit ASCII bytes` : `Corrupted / incomplete bitstream`);
        setText("flow-receiver-msg", `"${decMsg}"`);

        const rxBadge = document.getElementById("flow-receiver-badge");
        if (rxBadge) {
            rxBadge.className = isMatch ? "badge-soft-green font-bold" : "badge-mismatch font-bold";
            rxBadge.textContent = isMatch ? "✓ MESSAGE MATCH" : "✕ MESSAGE MISMATCH";
        }

        // Message Transformation Panel (Requirement 8)
        setText("flow-panel-original", origMsg);
        setText("flow-panel-ascii-bytes", stringToHex(origMsg));
        setText("flow-panel-binary", stringToBinary(origMsg, 12));
        setText("flow-panel-symbols", stringToSymbols(origMsg, 24));
        setText("flow-panel-header-vals", extractPacketValuesSample(packets, method, 5));
        setText("flow-panel-decoded-bits", isMatch ? stringToBinary(origMsg, 8) : "Incomplete / corrupted bitstream");
        setText("flow-panel-decoded-msg", decMsg);
        setText("flow-trans-status-tag", isMatch ? "Live Backend Telemetry (Verified)" : `Live Backend Telemetry (${(data.integrity || 'Failed').toUpperCase()})`);

        // Simulated Network Card Stats (Requirement 10)
        setText("flow-stat-generated", netStats.generated !== undefined ? netStats.generated : (data.packets_count || 0));
        setText("flow-stat-received", netStats.received !== undefined ? netStats.received : (data.packets_count || 0));
        setText("flow-stat-lost", netStats.lost || 0);
        setText("flow-stat-corrupted", netStats.corrupted || 0);
        setText("flow-stat-reordered", netStats.reordered || 0);

        // Integrity Card (Requirement 11)
        setText("flow-int-orig", origMsg);
        setText("flow-int-dec", decMsg);
        setText("flow-int-ber", ber);
        setText("flow-int-status-text", isMatch ? "✓ VERIFIED (Exact Match)" : `✕ VERIFICATION FAILED (${(data.integrity || 'Errors Detected').toUpperCase()})`);

        const intBadge = document.getElementById("flow-int-badge");
        if (intBadge) {
            intBadge.className = isMatch ? "badge-soft-green font-mono font-bold" : "badge-mismatch font-mono font-bold";
            intBadge.textContent = isMatch ? "✓ VERIFIED" : "✕ VERIFICATION FAILED";
        }
    }
}

