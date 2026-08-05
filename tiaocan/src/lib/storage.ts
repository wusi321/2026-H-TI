import type {
  RunMetadata,
  StoredSample,
  TaskProfileId,
  TelemetrySample,
  TuningParameters,
} from "../types";

const DB_NAME = "ball-balance-tuning";
const DB_VERSION = 1;
const RUNS_STORE = "runs";
const SAMPLES_STORE = "samples";
const PROFILES_STORE = "profiles";

function requestResult<T>(request: IDBRequest<T>): Promise<T> {
  return new Promise((resolve, reject) => {
    request.onsuccess = () => resolve(request.result);
    request.onerror = () => reject(request.error ?? new Error("IndexedDB request failed"));
  });
}

function transactionDone(transaction: IDBTransaction): Promise<void> {
  return new Promise((resolve, reject) => {
    transaction.oncomplete = () => resolve();
    transaction.onerror = () => reject(transaction.error ?? new Error("IndexedDB transaction failed"));
    transaction.onabort = () => reject(transaction.error ?? new Error("IndexedDB transaction aborted"));
  });
}

async function openDatabase(): Promise<IDBDatabase> {
  return new Promise((resolve, reject) => {
    const request = indexedDB.open(DB_NAME, DB_VERSION);
    request.onupgradeneeded = () => {
      const database = request.result;
      if (!database.objectStoreNames.contains(RUNS_STORE)) {
        database.createObjectStore(RUNS_STORE, { keyPath: "id" });
      }
      if (!database.objectStoreNames.contains(SAMPLES_STORE)) {
        const store = database.createObjectStore(SAMPLES_STORE, {
          keyPath: ["runId", "index"],
        });
        store.createIndex("byRun", "runId", { unique: false });
      }
      if (!database.objectStoreNames.contains(PROFILES_STORE)) {
        database.createObjectStore(PROFILES_STORE, { keyPath: "task" });
      }
    };
    request.onsuccess = () => resolve(request.result);
    request.onerror = () => reject(request.error ?? new Error("Unable to open IndexedDB"));
  });
}

export async function createRun(metadata: RunMetadata): Promise<void> {
  const database = await openDatabase();
  const transaction = database.transaction(RUNS_STORE, "readwrite");
  transaction.objectStore(RUNS_STORE).put(metadata);
  await transactionDone(transaction);
  database.close();
}

export async function appendSamples(
  runId: string,
  samples: TelemetrySample[],
): Promise<void> {
  if (samples.length === 0) return;
  const database = await openDatabase();
  const transaction = database.transaction(SAMPLES_STORE, "readwrite");
  const store = transaction.objectStore(SAMPLES_STORE);
  samples.forEach((sample) => {
    const stored: StoredSample = { runId, ...sample };
    store.put(stored);
  });
  await transactionDone(transaction);
  database.close();
}

export async function finishRun(
  runId: string,
  endedAtMs: number,
  sampleCount: number,
  actualTaskId?: number,
): Promise<void> {
  const database = await openDatabase();
  const transaction = database.transaction(RUNS_STORE, "readwrite");
  const store = transaction.objectStore(RUNS_STORE);
  const run = await requestResult(store.get(runId) as IDBRequest<RunMetadata | undefined>);
  if (run) store.put({ ...run, endedAtMs, sampleCount, actualTaskId });
  await transactionDone(transaction);
  database.close();
}

export async function listRuns(): Promise<RunMetadata[]> {
  const database = await openDatabase();
  const transaction = database.transaction(RUNS_STORE, "readonly");
  const runs = await requestResult(
    transaction.objectStore(RUNS_STORE).getAll() as IDBRequest<RunMetadata[]>,
  );
  await transactionDone(transaction);
  database.close();
  return runs.sort((a, b) => b.startedAtMs - a.startedAtMs);
}

export async function loadRunSamples(runId: string): Promise<TelemetrySample[]> {
  const database = await openDatabase();
  const transaction = database.transaction(SAMPLES_STORE, "readonly");
  const index = transaction.objectStore(SAMPLES_STORE).index("byRun");
  const stored = await requestResult(
    index.getAll(IDBKeyRange.only(runId)) as IDBRequest<StoredSample[]>,
  );
  await transactionDone(transaction);
  database.close();
  return stored
    .sort((a, b) => a.index - b.index)
    .map(({ runId: _runId, ...sample }) => sample);
}

export async function deleteRun(runId: string): Promise<void> {
  const database = await openDatabase();
  const transaction = database.transaction(
    [RUNS_STORE, SAMPLES_STORE],
    "readwrite",
  );
  transaction.objectStore(RUNS_STORE).delete(runId);
  const samples = transaction.objectStore(SAMPLES_STORE).index("byRun");
  const range = IDBKeyRange.only(runId);
  const cursorRequest = samples.openKeyCursor(range);
  cursorRequest.onsuccess = () => {
    const cursor = cursorRequest.result;
    if (!cursor) return;
    transaction.objectStore(SAMPLES_STORE).delete(cursor.primaryKey);
    cursor.continue();
  };
  await transactionDone(transaction);
  database.close();
}

export async function saveProfile(
  task: TaskProfileId,
  parameters: TuningParameters,
): Promise<void> {
  const database = await openDatabase();
  const transaction = database.transaction(PROFILES_STORE, "readwrite");
  transaction.objectStore(PROFILES_STORE).put({ task, parameters });
  await transactionDone(transaction);
  database.close();
}

export async function loadProfile(
  task: TaskProfileId,
): Promise<TuningParameters | null> {
  const database = await openDatabase();
  const transaction = database.transaction(PROFILES_STORE, "readonly");
  const record = await requestResult(
    transaction.objectStore(PROFILES_STORE).get(task) as IDBRequest<
      { task: TaskProfileId; parameters: TuningParameters } | undefined
    >,
  );
  await transactionDone(transaction);
  database.close();
  return record?.parameters ?? null;
}
